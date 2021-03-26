#pragma once

#include "CStreamer.h"
#include "CAM32.h"

class CAM32Streamer : public CStreamer
{
    bool m_showBig;
    CAM32 &m_cam;

public:
    CAM32Streamer(CAM32 &cam);

    virtual void    streamImage(uint32_t curMsec);
};
