
#include "CAM32Streamer.h"
#include <assert.h>



CAM32Streamer::CAM32Streamer(CAM32 &cam) : CStreamer(cam.getWidth(), cam.getHeight()), m_cam(cam)
{
    printf("Created streamer width=%d, height=%d\n", cam.getWidth(), cam.getHeight());
}

void CAM32Streamer::streamImage(uint32_t curMsec)
{
    m_cam.run();// queue up a read for next time

    BufPtr bytes = m_cam.getfb();
    streamFrame(bytes, m_cam.getSize(), curMsec);
    m_cam.done();
}
