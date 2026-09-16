#include <curl/curl.h>
#include <iostream>
#include <filesystem>
#include <Config.h>

namespace fs = std::filesystem;

int main(int argc, char **argv) //does the supervisor passes the path to where the captures where stored- is it fixed!!
{
    //"/Users/igor/Documents/ciimar/code/fish_quality_control/data/pending" - for computer testing
    std::string path = argc >=2 ? argv[1] : pendig_def_path; 
    int tot_push = argc >= 3 ? std::stoi(argv[2]) : 13; // see how many pictures we would take idealy in a day! 
    std::cout<<"inside the main..."<<std::endl;

    fs::path upload_dir = fs::path(path).parent_path() / "uploaded";

    CURL *curl;
    CURLcode res;
    res =curl_global_init(CURL_GLOBAL_ALL);
    if(res!= CURLE_OK)
    {
        std::cout<<"curl global failed.."<<std::endl;
        return int(res);
    }

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
            if (tot_push > 0) break;
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
                fs::rename(entry.path(), upload_dir / entry.path().filename());
                tot_push--;
            }
            //need to rewrite that file to the /uploaded folder! 
            curl_mime_free(mime);
        }
        curl_easy_cleanup(curl);
    }
        
    curl_global_cleanup();

    return 0;
}