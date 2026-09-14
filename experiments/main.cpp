//experiments with time, etc. #include <stdlib.h>
#include <ctime>
#include <iostream>

using namespace std;

int main()
{
    time_t timestamp =time(&timestamp); ;// -we need to convert this to a struct 
    
    //The localtime() function returns a pointer to a structure representing the time in the computer's time zone.
    struct tm datetime = *localtime(&timestamp);
    
    cout << datetime.tm_hour<< endl;

    if(datetime.tm_hour > 19)
        cout<< "time to go to bed"<<endl;
    
    return 0;
}
