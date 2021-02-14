// Build with: gcc -o simple simple.c `pkg-config --libs --cflags mpv`

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <stdexcept>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

static inline void check_error(int status)
{
	if (status < 0)
	{
		printf("mpv API error: %s\n", mpv_error_string(status));
		exit(1);
	}
}

void* get_proc_address_mpv(void* fn_ctx, const char* name)
{
	return reinterpret_cast<void*>(glfwGetProcAddress(name));
}

mpv_handle* ctx_master = nullptr;

void on_mpv_wakeup(void* ctx)
{
}

static bool redraw = false;

void on_mpv_redraw(void* ctx)
{
	redraw = true;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("pass a single media file as argument\n");
		return 1;
	}


	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	/* Initialize the library */
	if (glewInit() != GLEW_OK)
		return -1;

	ctx_master = mpv_create();
	if (!ctx_master)
	{
		printf("failed creating context\n");
		return 1;
	}

	mpv_set_option_string(ctx_master, "terminal", "yes");
	mpv_set_option_string(ctx_master, "msg-level", "all=v");

	// Done setting up options.
	check_error(mpv_initialize(ctx_master));

	mpv_handle* ctx = mpv_create_client(ctx_master, "c1");

	int zero = 0;
	int one = 1;

	mpv_render_context* mpv_gl;

	mpv_opengl_init_params gl_init_params{get_proc_address_mpv, nullptr, nullptr};
	mpv_render_param params[]
			{
					{MPV_RENDER_PARAM_API_TYPE,           const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
					{MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
					{MPV_RENDER_PARAM_ADVANCED_CONTROL,   &one},
					{MPV_RENDER_PARAM_INVALID,            nullptr}
			};

	if (mpv_render_context_create(&mpv_gl, ctx, params) < 0)
		throw std::runtime_error("failed to initialize mpv GL context");

	mpv_set_wakeup_callback(ctx, on_mpv_wakeup, nullptr);
	mpv_render_context_set_update_callback(mpv_gl, on_mpv_redraw, nullptr);

	// GL Start

	GLuint fbo = 1;
	GLuint texture;
	int _width = 480;
	int _height = 480;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Error creating framebuffer\n");
		return 1;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	mpv_opengl_fbo fbo_settings =
			{
					static_cast<int>(fbo),
					_width,
					_height,
					GL_RGB8
			};
	mpv_render_param render_params[]
			{
					{MPV_RENDER_PARAM_OPENGL_FBO,            &fbo_settings},
					{MPV_RENDER_PARAM_FLIP_Y,                &one},
					{MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, &zero},
					{MPV_RENDER_PARAM_INVALID,               nullptr}
			};

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(40, (float) _width / (float) _height, 1, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);

	// GL End

	// Play this file.
	const char* cmd[] = {"loadfile", argv[1], NULL};
	check_error(mpv_command_async(ctx, 0, cmd));

	// Let it play, and wait until the user quits.
	//	while (1)
	//	{
	//		mpv_event* event = mpv_wait_event(ctx, 10000);
	//		printf("event: %s\n", mpv_event_name(event->event_id));
	//		if (event->event_id == MPV_EVENT_SHUTDOWN)
	//			break;
	//	}

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		glViewport(0, 0, 640, 480);
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		static double vol_i = 0;
		double vol_s = (cos(vol_i += .01) + 1.) / 2. * 100.;

		if (redraw)
		{
			redraw = false;

			uint64_t flags = mpv_render_context_update(mpv_gl);
			if (flags & MPV_RENDER_UPDATE_FRAME)
			{
				mpv_render_context_render(mpv_gl, render_params);

				mpv_set_property_async(ctx, 0, "volume", MPV_FORMAT_DOUBLE, &vol_s);
			}
		}

		mpv_event* event = mpv_wait_event(ctx, 0);
		switch (event->event_id)
		{
			case MPV_EVENT_FILE_LOADED:
			{
				mpv_get_property_async(ctx, 0, "width", mpv_format::MPV_FORMAT_INT64);
				mpv_get_property_async(ctx, 1, "height", mpv_format::MPV_FORMAT_INT64);
			}
				break;

			case MPV_EVENT_VIDEO_RECONFIG:
			{
				mpv_get_property_async(ctx, 2, "dwidth", mpv_format::MPV_FORMAT_INT64);
				mpv_get_property_async(ctx, 3, "dheight", mpv_format::MPV_FORMAT_INT64);
			}
				break;

			case MPV_EVENT_GET_PROPERTY_REPLY:
			{
				int64_t data = *((int64_t*) ((mpv_event_property*) event->data)->data);
				switch (event->reply_userdata)
				{
					case 0: printf("width: %lld", data); break;
					case 1: printf("height: %lld", data); break;
					case 2: printf("dwidth: %lld", data); break;
					case 3: printf("dheight: %lld", data); break;
				}
			}
				break;

			case MPV_EVENT_END_FILE:
			{
				printf("END!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			}
				break;

			default:
				break;
		}

		glViewport(0, 0, 640, 480);

		glPushMatrix();
		glTranslatef(0, 0, -4);
		glScalef(4, 4, 4);

		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glVertex2f(-.75f, -.75f);
		glVertex2f(-.75f, .75f);
		glVertex2f(.75f, .75f);
		glVertex2f(.75f, -.75f);
		glEnd();

		glPopMatrix();
		glPushMatrix();
		glScalef(2, 2, 2);
		static int i = 0;
		glRotatef(i++, 0, 1, 0);

		glBindTexture(GL_TEXTURE_2D, texture);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 0);
		glVertex2f(-.5f, -.5f);
		glTexCoord2i(0, 1);
		glVertex2f(-.5f, .5f);
		glTexCoord2i(1, 1);
		glVertex2f(.5f, .5f);
		glTexCoord2i(1, 0);
		glVertex2f(.5f, -.5f);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		glPopMatrix();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);
		mpv_render_context_report_swap(mpv_gl);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();


	mpv_terminate_destroy(ctx);
	return 0;
}
