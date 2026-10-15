#include "WaterTank.hpp"


//TODO's
/*
    1- add timeouts so even if the sensors are not working, we can still perform this! ✅
    2- add the restart / reboot safety state  
    3- add a test function for the GPIOS before run the tank state update
    4- add a fuction that looks to diagnose the GPIOS everytime! maybe it can start working?... or still if we have this it can send us the error! 
    5- Check if we need to be calling always pumpOn for pumping the water- I thikn not bc this is a relay - check 
*/

WaterTank::WaterTank(uint8_t capture_timeout) : SENSOR_TIMEOUT(capture_timeout)//this runs with the object creation 
{
    //this runs when the object was created 
    //this->SENSOR_TIMEOUT = capture_timeout;
}

void init_gpios(gpio_ &gpio, int init_flag, bool is_sensor);
void aux_release_gpio(gpio_ &gpio);
int WaterTank::setup_gpios()//WaterTank
{
    set_next_state(tank_state::STABLE);
    this->gpio_active = false;
    /*
    sensor_down.offset = 144;
    sensor_up.offset = 43;
    pump.offset = 106;
    valve.offset = 105;
    
    init_gpios(sensor_down,GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN,true);
    init_gpios(sensor_up,GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN,true);

    init_gpios(pump,0,false);
    init_gpios(valve,0,false);
    */
    return 0;
}

void init_gpios(gpio_ &gpio, int init_flag, bool is_sensor)
{
    gpio.chip= gpiod_chip_open_by_name("gpiochip0");
    gpio.line= gpiod_chip_get_line(gpio.chip, gpio.offset);
    
    if(is_sensor)
        gpio.enabled = (gpiod_line_request_input_flags(gpio.line,"input",init_flag) == 0 );
    else
        gpio.enabled = (gpiod_line_request_output(gpio.line,"output",0) == 0);
}

bool WaterTank::setup_env()
{   
    switch (this->current_tank_state)
    {
        case tank_state::IDLE:
            this->pumpOFF();
            this->close_valve();
            if(sensor_up.val == 1)
            {
                set_next_state(tank_state::DRAIN);
                this->open_valve();
            }
            else if(sensor_down.val == 0 || sensor_up.val == 0) // It's EMPTY! - FILL
            {
                set_next_state(tank_state::FILL);
                this->pumpON();
            }
            break;
        
        case tank_state::DRAIN:
            if(sensor_up.val == 0)
            {
                this->close_valve();
                set_next_state(tank_state::STABLE);
            }
            else if(time_in_state() >= std::chrono::minutes(DRAIN_TIMEOUT))
            {
                set_next_state(tank_state::FILL);
                this->close_valve();
                this->pumpON();
            }
            break;

        case tank_state::FILL:
            if(time_in_state() >= std::chrono::minutes(FILL_TIMEOUT) || sensor_up.val == 1)
            {
                this->pumpOFF();
                set_next_state(tank_state::STABLE);
            }
            break;
    } 

   return this->current_tank_state == tank_state::STABLE;  
}

void WaterTank::read_sensors()
{
    this->sensor_up.val     =  gpiod_line_get_value(this->sensor_up.line);
    this->sensor_down.val   = gpiod_line_get_value(this->sensor_down.line);
    if(this->sensor_down.val == -1 || this->sensor_up.val == -1)
    {
        //trow error!...
    } 
}

void WaterTank::set_actuator(gpio_ &actuator, int val)
{
    gpiod_line_set_value(actuator.line,val);     
}

void WaterTank::close_valve()
{
    this->set_actuator(valve,0);//check if 0 is the val to turn off the valve! 
}

void WaterTank::open_valve()
{
    this->set_actuator(valve,1);//check if 1 is the val to turn off the valve! 
}

void WaterTank::pumpON()
{
    this->set_actuator(pump,1);//check if 0 is the val to turn off the valve! 
}
void WaterTank::pumpOFF()
{
    this->set_actuator(pump,0);//check if 0 is the val to turn off the valve! 
}




void aux_release_gpio(gpio_ &gpio)
{
    gpiod_line_release(gpio.line);   
    gpiod_chip_close(gpio.chip);
}
void WaterTank::release_gpio()
{
    aux_release_gpio(sensor_up);
    aux_release_gpio(sensor_down);
    aux_release_gpio(pump);
    aux_release_gpio(valve);
}

void WaterTank::update_state()//needs access to sensors! 
{       
    int st;
    int ret;
    switch(this->current_tank_state)
    {
        case tank_state::STABLE:
            /*this->close_valve();
            this->pumpOFF();NEED to check this!*/
            if(this->time_in_state() >= std::chrono::minutes(SENSOR_TIMEOUT))
            {
                //open valve 
                if(!gpio_active)//even if not active we have the TIMEOUTS! 
                {
                    set_next_state(tank_state::CAPTURE);
                    sensor_fork();
                }
                else
                {
                    this->open_valve();
                    set_next_state(tank_state::DRAIN);
                }
            }
            break;
        
        case tank_state::DRAIN:
            
            if( (time_in_state() >= std::chrono::minutes(DRAIN_TIMEOUT)) || (sensor_up.val == 0 && sensor_down.val == 0))
            {
                this->close_valve();
                this->pumpON();
                set_next_state(tank_state::FILL);
            }
            break;
        
        case tank_state::FILL: 
            if( (time_in_state()>= std::chrono::minutes(FILL_TIMEOUT)) || (sensor_up.val == 1 && sensor_down.val== 1))
            {
                this->pumpOFF();
                set_next_state(tank_state::CAPTURE);
                sens_pid= sensor_fork();
            }
            break;

        case tank_state::CAPTURE:
            this->close_valve(); // for safety- still check if
            this->pumpOFF(); // for safety
            
            ret = waitpid(sens_pid,&st,WNOHANG);
            if(ret > 0 )
                set_next_state(tank_state::STABLE);

            break;
    }
}

void WaterTank:: set_next_state(tank_state s )
{
    if(s != current_tank_state)
    {
        current_tank_state = s;
        state_entry = clk::now();
    }   
}

clk::duration WaterTank::time_in_state() const {
    return clk::now() - state_entry;
}


pid_t WaterTank::sensor_fork()
{
    
    pid_t p_id = fork();

    if(p_id)
        return p_id;
    
    char* argv_sen[] =
    {
        (char*)"/home/ciimar/fish_quality_control/env/bin/python3",   
        (char*)"/home/ciimar/fish_quality_control/sensor_capture/src/I4FSensReadRawData.py",
        (char*)"-c",
        (char*)"/home/ciimar/fish_quality_control/sensor_capture/config/sensors_config.json",
        (char*)"-o",
        (char*)"/home/ciimar/fish_quality_control/data/pending",
        nullptr
    };
    execv("/home/ciimar/fish_quality_control/env/bin/python3", argv_sen);
    // only reached if execv FAILED:
    perror("execv sensor");
    _exit(127);
}


void WaterTank::test_gpios()
{
    const int toggle_delay = 5; // seconds — change as needed

    read_sensors();

    cout << "\n--- GPIO DIAGNOSTIC ---" << endl;
    cout << "SENSOR_UP   (offset " << sensor_up.offset   << ")  : val=" << sensor_up.val   << "  enabled=" << boolalpha << sensor_up.enabled   << endl;
    cout << "SENSOR_DOWN (offset " << sensor_down.offset  << ")  : val=" << sensor_down.val  << "  enabled=" << boolalpha << sensor_down.enabled  << endl;

    cout << "\nPUMP (offset " << pump.offset << "):" << endl;
    cout << "  ON..."  << endl;
    pumpON();
    sleep(toggle_delay);
    cout << "  OFF..." << endl;
    pumpOFF();
    sleep(toggle_delay);

    cout << "\nVALVE (offset " << valve.offset << "):" << endl;
    cout << "  OPEN..."  << endl;
    open_valve();
    sleep(toggle_delay);
    cout << "  CLOSE..." << endl;
    close_valve();
    sleep(toggle_delay);

    cout << "--- END DIAGNOSTIC ---\n" << endl;
}