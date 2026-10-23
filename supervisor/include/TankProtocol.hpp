#include "protocol.h"
#include "FrameParser.h"
#include "LinuxSerialTransport.hpp"

#include <string.h>

enum class com_state
{
    IDLE,
    WAIT_RESPONSE,
};

class TankProtocol {

    
private:
    ITransport* transport;
    NodeId myId;

    void buildHeader(Frame& f, MsgType type, uint8_t len, NodeId dest);

public:

    void init(ITransport* t, NodeId id);
    void update_com();
    com_state current_com_state;

    // Convenience senders
    int16_t sendPing();
    int16_t sendWakeUp();
    int16_t sendStartCycle();
    int16_t sendSetTimeout(uint8_t cmdId, uint32_t timeout_ms);
    int16_t askTankState();           // sends the DATA_ASK


    int16_t send(Frame& f, uint8_t expectedType, uint32_t timeout_ms, uint8_t n_tries);

    // Receiving
    void update();
    bool hasNewFrame();
    bool poll_pong();
    bool poll_tank_ready(bool& ready); // pumps, returns true if a reply came; sets ready
    void getFrame(Frame& f);
};
