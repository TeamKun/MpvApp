// Build with: gcc -o simple simple.c `pkg-config --libs --cflags mpv`

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <stdexcept>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

static inline void check_error(int status)
{
	if (status < 0)
	{
		printf("mpv API error: %s\n", mpv_error_string(status));
		exit(1);
	}
}

void* get_proc_address(void* ctx, const char* name)
{
	return (void*) glfwGetProcAddress(name);
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


	mpv_handle* ctx = mpv_create();
	if (!ctx)
	{
		printf("failed creating context\n");
		return 1;
	}

	// Enable default key bindings, so the user can actually interact with
	// the player (and e.g. close the window).
	check_error(mpv_set_option_string(ctx, "input-default-bindings", "yes"));
	mpv_set_option_string(ctx, "input-vo-keyboard", "yes");
	int val = 1;
	check_error(mpv_set_option(ctx, "osc", MPV_FORMAT_FLAG, &val));

	// Done setting up options.
	check_error(mpv_initialize(ctx));

	// Play this file.
	const char* cmd[] = {"loadfile", argv[1], NULL};
	check_error(mpv_command(ctx, cmd));

	mpv_render_context* mpv_gl;

	mpv_opengl_init_params gl_init_params{get_proc_address, nullptr, nullptr};
	mpv_render_param params[]{
			{MPV_RENDER_PARAM_API_TYPE,           const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
			{MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
			{MPV_RENDER_PARAM_INVALID,            nullptr}
	};

	if (mpv_render_context_create(&mpv_gl, ctx, params) < 0)
		throw std::runtime_error("failed to initialize mpv GL context");

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
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();


	mpv_terminate_destroy(ctx);
	return 0;
}