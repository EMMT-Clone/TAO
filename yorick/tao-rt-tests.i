/*
 * tao-rt-tests.i --
 *
 * Tests and examples using Yorick interface to TAO real-time software.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018-2019, Éric Thiébaut.
 */

require, "tao-rt.i";

func tao_measure_latency(cam, number=, timeout=, semaphore=)
/* DOCUMENT dt = tao_measure_latency(cam);

     Yield an array of time latencies (in seconds) when receiving images from
     a TAO camera server.  CAM is a TAO shared camera or the shared memory
     identifier of a TAO shared camera.

     Keyword NUMBER can be used to specify the number of images to acquire.

     Keywords TIMEOUT and SEMAPHORE can be used to specify the timeout in
     seconds and semaphore number when waiting for images.

   SEE ALSO: tao_attach_shared_camera, tao_wait_image, window, pli.
 */

{
    if (is_integer(cam)) cam = tao_attach_shared_camera(cam);
    if (is_void(semaphore)) semaphore = 1; // use 1st semaphore by default
    if (is_void(number)) number = 100;
    if (is_void(timeout)) timeout = 1.0;
    out = array(double, number);
    for (i = 1; i <= number; ++i) {
        arr = tao_wait_image(cam, semaphore, timeout);
        t1 = tao_get_monotonic_time();
        t0 = arr.timestamp;
        dt = t1 - t0;
        out(i) = dt(1) + dt(2)*1e-9;
    }
    return out;
}

func tao_live(cam, number=, timeout=, semaphore=, cmin=, cmax=, win=, wait=)
/* DOCUMENT tao_live, cam;

     Display images from a TAO camera server.  CAM is a TAO shared camera or
     the shared memory identifier of a TAO shared camera.

     Keyword NUMBER can be used to specify the number of images to display.

     Keywords TIMEOUT and SEMAPHORE can be used to specify the timeout in
     seconds and semaphore number when waiting for images.

     Keywords WIN, CMIN and CMAX can be used to set the window and the levels
     for displaying the images.

   SEE ALSO: tao_attach_shared_camera, tao_wait_image, window, pli.
 */
{
    if (! is_void(win)) window, win;
    if (is_integer(cam)) cam = tao_attach_shared_camera(cam);
    if (is_void(wait)) wait = 0.1; // wait 0.1 seconds
    if (is_void(semaphore)) semaphore = 1; // use 1st semaphore by default
    if (is_void(number)) number = 100000;
    if (is_void(timeout)) timeout = 1.0;
    for (i = 1; i <= number; ++i) {
        arr = tao_wait_image(cam, semaphore, timeout);
        if (is_void(arr)) {
            write, format="%s\n", "Timeout!";
            continue;
        }
        fma;
        pli, arr.data, cmin=cmin, cmax=cmax;
        pltitle, swrite(format="Frame %d", arr.counter);
        pause, wait;
    }
}
