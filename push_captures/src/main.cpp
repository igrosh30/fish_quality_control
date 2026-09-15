#include <curl/curl.h>
#include <iostream>
#include <filesystem>
#include <Config.h>

namespace fs = std::filesystem;

int main(int argc, char **argv) //does the supervisor passes the path to where the captures where stored- is it fixed!!
{

    std::string path = argc >1 ? argv[1] : "";  
    
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
        curl_easy_setopt(curl,CURLOPT_URL,"http://localhost:8000/push");//further need to understand how we'll reach the server - we do have VPN in the CIIMAR 

        
        int tot_pushed = 0;
        for(const auto& entry: fs::directory_iterator(path))
        {
            if (!entry.is_regular_file()) continue;             
            if (tot_pushed >= 10) break;
            const char* filepath = entry.path().c_str();

            curl_mime *mime =curl_mime_init(curl); //the body to send over HTTP
            /*Creating a slot inside the body and adding components*/
            curl_mimepart *part= curl_mime_addpart(mime);
            curl_mime_name(part,"file");
            
            //need to compute the path! 
            curl_mime_filedata(part,filepath);
            
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);//Attach the body to the post request
            
            /*Perfomr the request, res gets the return code*/
            res = curl_easy_perform(curl);
            if(res == CURLE_OK) tot_pushed++;
            //need to rewrite that file to the /uploaded folder! 
            curl_mime_free(mime);
        }
        curl_easy_cleanup(curl);
    }
        
    curl_global_cleanup();

    return 0;
}