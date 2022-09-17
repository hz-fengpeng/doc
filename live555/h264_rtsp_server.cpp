#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>

static char const* inputFileName = "test.h264";

int main(int argc, char* argv[])
{
    /* 创建调度器 */
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    /* 创建rtsp服务器 */
    RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554);
    if (rtspServer == NULL)
    {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        return -1;
    }

    /* 创建会话 */
    ServerMediaSession* sms = ServerMediaSession::createNew(*env, "live", "test", "test");
    
    /* 添加子会话 */
    sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(*env, inputFileName, True));
    
    /* 向服务器添加会话 */
    rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);
	*env << "Play this stream using the URL \"" << url << "\"\n";
	delete[] url;

    /* 循环处理事件 */
    env->taskScheduler().doEventLoop();

    return 0;
}
