#include <stdio.h>
#include <string>

int main(int argc, char* argv[])
{
    FILE* sdp_file = fopen(argv[1], "r");
    if(sdp_file == NULL){
        printf("open file failed!\n");
        return 0;
    }

    const int maxSdpLen = 10240;
    char sdp[maxSdpLen];
    fgets(sdp, maxSdpLen, sdp_file);
    if(!feof(sdp_file)){
        printf("sdp large than 10k!\n");
        return 0;
    }

    std::string sdpString(sdp);
    FILE* res_sdp_file = fopen("out.sdp", "w");
    if(res_sdp_file==NULL){
        printf("open output file fail!\n");
        return 0;
    }
    std::string split("\\r\\n");
    std::string sub;
    int start, end = -1*split.size();

    do{
        start = end + split.size();
        end = sdpString.find(split, start);
        sub = sdpString.substr(start, end -start);
        fwrite(sub.c_str(), sub.size(), 1, res_sdp_file);
        fwrite("\n", 1, 1, res_sdp_file);
        //printf("%s\n", sub.c_str());
    }while(end != -1);


    fclose(res_sdp_file);
    fclose(sdp_file);



}