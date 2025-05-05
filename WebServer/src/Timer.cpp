#include "Timer.h"

TimerNode::TimerNode(std::shared_ptr<HttpData> httpData, int milli_timeout)
    : isValid_(true), httpData_(httpData)
{
    expire_time_point = std::chrono::system_clock::now() + std::chrono::milliseconds(milli_timeout);
}

TimerNode::~TimerNode()
{
    if (httpData_.lock())
    {
        std::shared_ptr<HttpData> httpData = httpData_.lock();
        httpData->handleClose(); // handle the close event
    }
    httpData_.reset(); // reset the weak pointer
    isValid_ = false;
}

TimerNode::TimerNode(const TimerNode& tn)
{
    this->httpData_ = tn.httpData_;
    this->expire_time_point = std::chrono::system_clock::now();
    this->isValid_ = true;
}

TimerNode& TimerNode::operator=(const TimerNode& tn)
{
    if (this != &tn)
    {
        this->httpData_ = tn.httpData_;
        this->expire_time_point = std::chrono::system_clock::now();
        this->isValid_ = true;
    }
    return *this;
}

void TimerNode::update(int milli_timeout)
{
    expire_time_point = std::chrono::system_clock::now() + std::chrono::milliseconds(milli_timeout);
}

bool TimerNode::isValid()
{
    auto now = std::chrono::system_clock::now();
    if (now < expire_time_point)
    {
        return true;
    }
    else
    {
        this->expired();
        return false;
    }
}

void TimerNode::clearHttpData()
{
    httpData_.reset();
    this->expired();
}

void TimerNode::expired()
{
    isValid_ = false;
}

bool TimerNode::isDeleted()
{
    return !isValid_;
}

std::chrono::system_clock::time_point TimerNode::getExpireTime()
{
    return expire_time_point;
}

void TimerManager::addTimerNode(std::shared_ptr<HttpData> httpData, int milli_timeout)
{
    std::shared_ptr<TimerNode> newNode = std::make_shared<TimerNode>(httpData, milli_timeout);
    timerQueue.push(newNode);
    httpData->linkTimer(newNode);
}

void TimerManager::handleExpiredEvent()
{
    while (!timerQueue.empty())
    {
        std::shared_ptr<TimerNode> tn = timerQueue.top(); // get the top element
        if (tn->isDeleted())                              // if the top element is deleted
        {
            timerQueue.pop(); // pop the top element
        }
        else if (!(tn->isValid()))
        {
            tn->clearHttpData(); // clear the weak pointer
            timerQueue.pop();    // pop the top element
        }
        else
        {
            break; // if not expired, exit the loop
        }
    }
}
