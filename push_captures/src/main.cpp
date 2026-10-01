#include <curl/curl.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <Config.h>

using namespace std;
using namespace std::filesystem;

int main(int argc, char **argv) //does the supervisor passes the path to where the captures where stored- is it fixed!!
{
    //"/Users/igor/Documents/ciimar/code/fish_quality_control/data/pending" - for computer testing
    int asked2push = argc >= 2 ? std::stoi(argv[1]) : 26; // see how many pictures we would take idealy in a day! 
    path dir_path_pending  = argc >=3 ? argv[2] : pendig_def_path; 
    int tot_uploads = 0;
    int attempted     = 0;   // regular files we actually tried
    int connect_fails = 0;   // transport-level failures (never reached the server)
    
    
    cout<<"received "<<asked2push<< " as the number of pictures to push"<<endl;
    //check dir to pending folder: I can also check the tot_push! 
    if(!exists(dir_path_pending) || !is_directory(dir_path_pending))
    {
        //trow error that file doesn't exist! 
        cout<<"path: " <<dir_path_pending<< " doesn't exist!"<<endl;
        return static_cast<int>(PushResult::FOLDER_OFF);//the code for the path passed doesn't exist! 
    }

    path dir_path_uploaded = dir_path_pending.parent_path() / "uploaded";

    CURL *curl;
    CURLcode res;
    res =curl_global_init(CURL_GLOBAL_ALL);
    if(res!= CURLE_OK)
        return static_cast<int>(PushResult::LOAD_CULR_FAILED);//see the right error code for this!

    //creates an handle for a transfer
    curl = curl_easy_init();
    if(curl)
    {    
        //set the URL that will receive the POST
        
        curl_easy_setopt(curl,CURLOPT_URL,"http://192.168.220.175:8000/push");//further need to understand how we'll reach the server - we do have VPN in the CIIMAR 
        curl_easy_setopt(curl,CURLOPT_VERBOSE,1L);
        curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,5L); // fail fast if the host is dead (LAN; tune this)

        for(const auto& entry: directory_iterator(dir_path_pending))//->C++17!
        { 
            if (!entry.is_regular_file()) continue;          
            attempted++;   
            //It needs to drain the file! - imagine that we have a huge data - need to go over it
            // if (tot_push <= 0)
            // {
            //     std::cout<<"tot_push < 0 {"<<tot_push<<"}"<<std::endl;
            //     break;
            // }
            
            const char* filepath = entry.path().c_str();

            curl_mime *mime= curl_mime_init(curl); //the body to send over HTTP
            /*Creating a slot inside the body and adding components*/
            curl_mimepart *part= curl_mime_addpart(mime);
            curl_mime_name(part,"file");
            curl_mime_filedata(part,filepath);
            
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);//Attach the body to the post request
            
            /*Perfomr the request, res gets the return code*/
            res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (res == CURLE_OK && http_code == 201)
            {
                std::cout<<"POST_OK"<<std::endl;
                rename(entry.path(), dir_path_uploaded / entry.path().filename());
                tot_uploads++;
            }
            else if (res == CURLE_COULDNT_CONNECT  || res == CURLE_COULDNT_RESOLVE_HOST ||
                     res == CURLE_OPERATION_TIMEDOUT)
            {
                connect_fails++; // never reached the server
            }
            

            //need to rewrite that file to the /uploaded folder! 
            curl_mime_free(mime);
        }
        curl_easy_cleanup(curl);
    }
    else{
        //Pass the code for this error!
        return static_cast<int>(PushResult::CURL_INIT_FAILED);
    }
        
    curl_global_cleanup();

    //check the problems here!
    if (attempted > 0 && tot_uploads == 0 && connect_fails == attempted)
        return static_cast<int>(PushResult::SERVER_OFF);
    
    if (tot_uploads == attempted)
        return static_cast<int>(PushResult::SUCCESS);

    return static_cast<int>(PushResult::INCOMPLETE);
}