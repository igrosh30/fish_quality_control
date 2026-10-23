#include "TankProtocol.hpp"

void TankProtocol::init(ITransport* t, NodeId id)
{
    this->transport = t;
    this->myId=id;
}

void TankProtocol::buildHeader(Frame& f, MsgType type, uint8_t len, NodeId dest)
{
    f.sof         = SOF_VALUE;
    f.header.src  = myId;
    f.header.dest = dest;
    f.header.type = type;
    f.header.len  = len;
}

// void TankProtocol::update_com()
// {
//     switch (current_com_state)
//     {
//     case IDLE:
//         /* code */
//         break;
    
//     default:
//         break;
//     }
// }

int16_t TankProtocol::send(Frame &f,uint8_t expectedType ,uint32_t timeout, uint8_t n_tries)
{
    transport->sendFrame(f);


}

int16_t TankProtocol::sendPing()
{
    if(transport == nullptr)
        return -1;
    Frame f;
    buildHeader(f, PING, 0, TANK_MCU);
    return transport->sendFrame(f);
}
bool TankProtocol::poll_pong()
{
    if (transport == nullptr) return false;
    transport->updateFrame();
    if (transport->hasNewFrame()) {
        Frame f;
        transport->getFrame(f);
        return (f.header.type == PONG);
    }
    return false;
}


int16_t TankProtocol::sendWakeUp()
{
    if(transport == nullptr)
        return -1;
    
    Frame f;
    buildHeader(f, CMD, 1, TANK_MCU);
    f.payload.data.cmdId = WAKE_UP;
    return transport->sendFrame(f);
}

int16_t TankProtocol::sendStartCycle()
{
    if(transport == nullptr)
        return -1;
        
    Frame f;
    buildHeader(f, CMD, 1, TANK_MCU);
    f.payload.data.cmdId = START_CYCLE;
    return transport->sendFrame(f);
}

int16_t TankProtocol::sendSetTimeout(uint8_t cmdId, uint32_t timeout_ms)
{
    if(transport == nullptr)
        return -1;

    Frame f;
    buildHeader(f, CMD, 5, TANK_MCU);
    f.payload.data.cmdId = cmdId;
    memcpy(f.payload.data.param, &timeout_ms, sizeof(timeout_ms));
    return transport->sendFrame(f);
}

int16_t TankProtocol::askTankState()
{
    if (transport == nullptr) return -1;
    Frame f;
    buildHeader(f, DATA_ASK, 1, TANK_MCU);
    f.payload.data.cmdId = ;
    return transport->sendFrame(f);
}

void TankProtocol::update()
{
    if(transport == nullptr)
        return;

    transport->updateFrame();
}

bool TankProtocol::hasNewFrame()
{
    if(transport == nullptr)
        return false;

    return transport->hasNewFrame();
}

void TankProtocol::getFrame(Frame& f)
{
    if(transport == nullptr)
        return;

    transport->getFrame(f);
}
