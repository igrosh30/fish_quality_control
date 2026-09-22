//experiments with time, etc.  #include <stdlib.h>
#include <ctime>
#include <iostream>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>

using namespace std;
using namespace std::filesystem;

int main(int argc, char **argv) //does the supervisor passes the path to where the captures where stored- is it fixed!!
{
    //"/Users/igor/Documents/ciimar/code/fish_quality_control/data/pending" - for computer testing
    int tot_push = argc >= 2 ? std::stoi(argv[1]) : 26; // see how many pictures we would take idealy in a day! 
    path dir_pending_path = argc >=3 ? argv[2] : "/Users/igor/Documents/ciimar/code/fish_quality_control/data/pending" ; 
    int tot_uploads = 0;

    if(!exists(dir_pending_path))
    {
        create_directory(dir_pending_path);
        cout << "Directory created @ " << dir_pending_path<<endl;
        return -1; //-1 meaning error- dir not existed! 
    }

    if(is_directory(dir_pending_path))
    {
        //get the parent path:
        path parent_path = dir_pending_path.parent_path();
        cout<<"parend path: "<<parent_path<<endl;
        
        //path for rewriting:
        
        path dir_uploaded_path;
        if(exists(parent_path / "uploaded"))
        {
            dir_uploaded_path= parent_path / "uploaded";
            cout<<"Uploaded path: "<< dir_uploaded_path<<endl;
        }
        else 
            cout<<"error: uploaded path dir "<<endl;

        int i = 0;
        for(const auto &entry: directory_iterator(dir_pending_path))
        {    
            cout<<"iteration: "<<i++<<endl;
            cout<<"File: " << entry.path()<<endl;
        }

    }
    else
    {
        cout<<"not a valid dir path!"<<endl;
    }
    
    /*
    CURL *curl;
    CURLcode res;
    res =curl_global_init(CURL_GLOBAL_ALL);
    if(res!= CURLE_OK)
        return int(res);

    //creates an handle for a transfer
    curl = curl_easy_init();
    if(curl)
    {    
        //set the URL that will receive the POST
        
        curl_easy_setopt(curl,CURLOPT_URL,"http://192.168.220.175:8000/push");//further need to understand how we'll reach the server - we do have VPN in the CIIMAR 
        curl_easy_setopt(curl,CURLOPT_VERBOSE,1L);

        for(const auto& entry: fs::directory_iterator(path))
        {
            
            if (!entry.is_regular_file()) continue;             
            if (tot_push <= 0)
            {
                std::cout<<"tot_push < 0 {"<<tot_push<<"}"<<std::endl;
                break;
            }
            
            const char* filepath = entry.path().c_str();

            curl_mime *mime= curl_mime_init(curl); //the body to send over HTTP
            //Creating a slot inside the body and adding components/
            curl_mimepart *part= curl_mime_addpart(mime);
            curl_mime_name(part,"file");
            curl_mime_filedata(part,filepath);
            
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);//Attach the body to the post request
            
            //Perfomr the request, res gets the return code
            res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (res == CURLE_OK && http_code == 201)
            {
                std::cout<<"POST_OK"<<std::endl;
                fs::rename(entry.path(), upload_dir / entry.path().filename());
                tot_uploads++;
                tot_push--;
            }
            else
            {
                //POST Error! 
                std::cout<<"post error..."<<std::endl;
            }
            //need to rewrite that file to the /uploaded folder! 
            curl_mime_free(mime);
        }
        curl_easy_cleanup(curl);
    }
        
    curl_global_cleanup();
    */
    return 0;
}



/*
 time_t timestamp =time(&timestamp); ;// -we need to convert this to a struct 
    
    //The localtime() function returns a pointer to a structure representing the time in the computer's time zone.
    struct tm datetime = *localtime(&timestamp);
    
    cout << datetime.tm_hour<< endl;

    if(datetime.tm_hour > 19)
        cout<< "time to go to bed"<<endl;
*/
