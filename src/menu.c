#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <xcb/xcb.h> 
#define WIDTH 300
#define HEIGHT 100

static xcb_gc_t getFontGC (xcb_connection_t *c,
						   	xcb_screen_t *screen,
							xcb_window_t window,
							const char *font_name );

static void drawText (xcb_connection_t *c,
					xcb_screen_t *screen,
					xcb_window_t window,
					int16_t x1,
					int16_t y1,
					const char *label );

static void
testCookie (xcb_void_cookie_t cookie,
            xcb_connection_t *connection,
            char *errMessage )
{
    xcb_generic_error_t *error = xcb_request_check (connection, cookie);
    if (error) {
        fprintf (stderr, "ERROR: %s : %"PRIu8"\n", errMessage , error->error_code);
        xcb_disconnect (connection);
        exit (-1);
    }
}

static void
drawText (xcb_connection_t  *connection,
           xcb_screen_t     *screen,
           xcb_window_t      window,
           int16_t           x1,
           int16_t           y1,
           const char       *label )
{

    /* get graphics context */
    xcb_gcontext_t gc = getFontGC (connection, screen, window, "fixed");


    /* draw the text */
    xcb_void_cookie_t textCookie = xcb_image_text_8_checked (connection,
                                                             strlen (label),
                                                             window,
                                                             gc,
                                                             x1, y1,
                                                             label );

    testCookie(textCookie, connection, "can't paste text");


    /* free the gc */
    xcb_void_cookie_t gcCookie = xcb_free_gc (connection, gc);

    testCookie(gcCookie, connection, "can't free gc");
}


static xcb_gc_t
getFontGC (xcb_connection_t  *connection,
           xcb_screen_t      *screen,
           xcb_window_t       window,
           const char        *font_name )
{
    /* get font */
    xcb_font_t font = xcb_generate_id (connection);
    xcb_void_cookie_t fontCookie = xcb_open_font_checked (connection,
                                                          font,
                                                          strlen (font_name),
                                                          font_name );

    testCookie(fontCookie, connection, "can't open font");


    /* create graphics context */
    xcb_gcontext_t  gc            = xcb_generate_id (connection);
    uint32_t        mask          = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
    uint32_t        value_list[3] = { screen->black_pixel,
                                      screen->white_pixel,
                                      font };

    xcb_void_cookie_t gcCookie = xcb_create_gc_checked (connection,
                                                        gc,
                                                        window,
                                                        mask,
                                                        value_list );

    testCookie(gcCookie, connection, "can't create gc");


    /* close font */
    fontCookie = xcb_close_font_checked (connection, font);

    testCookie(fontCookie, connection, "can't close font");

    return gc;
}

static void
setWindowProperties (xcb_connection_t 	*connection,
					xcb_window_t 		window,
					char 				*title)
{
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
                    window, XCB_ATOM_WM_NAME,
                    XCB_ATOM_STRING,
                    8, strlen(title),title);
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
                    window, XCB_ATOM_WM_CLASS,
                    XCB_ATOM_STRING,
                    8, strlen(title),title);

    // USE ATOMS to DEFINE PROPERTIES
    xcb_atom_t window_type_atom, window_type_dock_atom;
        xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(connection, 0, strlen("_NET_WM_WINDOW_TYPE"), "_NET_WM_WINDOW_TYPE");
        xcb_intern_atom_reply_t *atom_reply = xcb_intern_atom_reply(connection, atom_cookie, NULL);
        if (!atom_reply) {
             fprintf(stderr, "Unable to set window type. You will need to manually set your window manager to run lighthouse as you'd like.");
        } else {
            window_type_atom = atom_reply->atom;
            free(atom_reply);

            atom_cookie = xcb_intern_atom(connection, 0, strlen("_NET_WM_WINDOW_TYPE_DOCK"), "_NET_WM_WINDOW_TYPE_DOCK");
            atom_reply = xcb_intern_atom_reply(connection, atom_cookie, NULL);
            if (atom_reply) {
                window_type_dock_atom = atom_reply->atom;
                free(atom_reply);
                xcb_change_property_checked(connection, XCB_PROP_MODE_REPLACE, window, window_type_atom, XCB_ATOM_ATOM, 32, 1, &window_type_dock_atom);
            } else {
                fprintf(stderr, "Unable to set window type. You will need to manually set your window manager to run lighthouse as you'd like.");
            }
      }

	return;
}


int
main ()
{
    // get the connection
    int screenNum;
    xcb_connection_t *connection = xcb_connect (NULL, &screenNum);
    if (!connection) {
        fprintf (stderr, "ERROR: can't connect to an X server\n");
        return -1;
    }


    // get the current screen
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator (xcb_get_setup (connection));

    xcb_screen_t *screen = iter.data;

	// Check to see if the screen could be found
    if (!screen) {
        fprintf (stderr, "ERROR: can't get the current screen\n");
        xcb_disconnect (connection);
        return -1;
    }


    // create the window
    xcb_window_t window = xcb_generate_id (connection);

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[2];
    values[0] = screen->white_pixel;
    values[1] = XCB_EVENT_MASK_KEY_RELEASE |
                XCB_EVENT_MASK_KEY_PRESS |
                XCB_EVENT_MASK_BUTTON_PRESS |
                XCB_EVENT_MASK_EXPOSURE |
                XCB_EVENT_MASK_POINTER_MOTION;

    values[0] = screen->width_in_pixels / 2 - WIDTH/2;
    values[1] = screen->height_in_pixels / 2 - HEIGHT/2;

    xcb_void_cookie_t windowCookie = xcb_create_window_checked (connection,
                                                                screen->root_depth,
                                                                window, screen->root,
                                                                values[0], values[1],
                                                                WIDTH, HEIGHT,
                                                                10, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                                                screen->root_visual,
                                                                mask, values);

    testCookie(windowCookie, connection, "can't create window");

    // Name the window
    char *title = "BMenu";
	setWindowProperties(connection,window,title);

	// Map the window and check to see if it worked
    xcb_void_cookie_t mapCookie = xcb_map_window_checked (connection, window);
    testCookie(mapCookie, connection, "can't map window");

	// Draw the window
    xcb_flush(connection); 

    // event loop
    xcb_generic_event_t  *event;
    while (1) { ;
        if ( (event = xcb_poll_for_event(connection)) ) {
            switch (event->response_type & ~0x80) {
                case XCB_EXPOSE: {

                    // Get the input focus on initial start-up
                    xcb_void_cookie_t focus_cookie = xcb_set_input_focus_checked(connection, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
                    testCookie(focus_cookie, connection, "Failed to grab focus.");
                    
                    // Draw some text
                    drawText (connection, 
                              screen,
                              window,
                              WIDTH/2-strlen("BMenu"), HEIGHT/2,
                              "BMenu" );
                    break;
                }
				case XCB_ENTER_NOTIFY: {
					// Check to see if mouse entered the window
					// and set the window to be in focus

					xcb_void_cookie_t focus_cookie = xcb_set_input_focus_checked(connection, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
					testCookie(focus_cookie, connection, "Failed to grab focus.\n");
				}
                case XCB_KEY_RELEASE: { //Exit program if escape is pressed
					// Map key release event
                    xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;

					// Check to see if escape key was pressed
                    switch (kr->detail) {
                        // ESC key
                        case 9: {
                            free (event);
                            xcb_disconnect (connection);
                            return 0;
                        }
                    }
                    free (event);
                }
                case XCB_EVENT_MASK_BUTTON_PRESS: {

                    // Get the input focus
                    xcb_void_cookie_t focus_cookie = xcb_set_input_focus_checked(connection, XCB_INPUT_FOCUS_POINTER_ROOT,window, XCB_CURRENT_TIME);
                    testCookie(focus_cookie, connection, "Failed to grab focus.\n");

					// Temporary check to see if the window can be moved
					values[0] = values[0] -10;
					values[1] = values[1] -10;
					xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
					xcb_void_cookie_t mapCookie = xcb_map_window_checked (connection, window);
					testCookie(mapCookie, connection, "Can't move the window, click \n");
					xcb_flush(connection);
					break;
                }
            }
        }
    }
    return 0;
}
