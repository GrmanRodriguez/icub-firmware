/*
 * Copyright (C) 2017 iCub Facility - Istituto Italiano di Tecnologia
 * Author:  Marco Accame
 * email:   marco.accame@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/


// --------------------------------------------------------------------------------------------------------------------
// - public interface
// --------------------------------------------------------------------------------------------------------------------

#include "embot_os_Thread.h"


// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "embot_os_rtos.h"


// - class InitThread

// --------------------------------------------------------------------------------------------------------------------
// - pimpl: private implementation (see scott meyers: item 22 of effective modern c++, item 31 of effective c++
// --------------------------------------------------------------------------------------------------------------------

struct embot::os::InitThread::Impl
{ 
    bool _started {false}; 
    bool _terminated {false};     
    embot::os::rtos::thread_t *rtosthread {nullptr};     
    Impl() = default;    
    ~Impl() { }       
};


// --------------------------------------------------------------------------------------------------------------------
// - all the rest
// --------------------------------------------------------------------------------------------------------------------


embot::os::InitThread& embot::os::InitThread::getInstance()
{
    static InitThread* p = new InitThread();
    return *p;
}

embot::os::InitThread::InitThread()
{   
    pImpl = std::make_unique<Impl>();    
}


embot::os::InitThread::~InitThread() { }


embot::os::Thread::Type embot::os::InitThread::getType() const 
{
    return Type::Init;
}


embot::os::Priority embot::os::InitThread::getPriority() const
{   
    return embot::os::Priority::schedInit;
}


bool embot::os::InitThread::setPriority(embot::os::Priority priority)
{
    return false;
}

const char * embot::os::InitThread::getName() const
{
   return "tINIT"; 
}

  
bool embot::os::InitThread::setEvent(embot::os::Event event)
{
    return false;
}  


bool embot::os::InitThread::setMessage(embot::os::Message message, core::relTime timeout)
{
    return false;
}

bool embot::os::InitThread::setValue(embot::os::Value value, core::relTime timeout)
{
    return false;
}

bool embot::os::InitThread::setCallback(const core::Callback &callback, core::relTime timeout)
{
    return false;
}

void embot::os::InitThread::synch()
{    
    pImpl->rtosthread = embot::os::rtos::scheduler_thread_running();
    embot::os::rtos::scheduler_associate(pImpl->rtosthread, this);    
    pImpl->_started = true;    
}

void embot::os::InitThread::run() {}
    

void embot::os::InitThread::terminate()
{
    pImpl->_terminated = true;
}

bool embot::os::InitThread::isterminated() const
{
    return pImpl->_terminated;
}
//        
//void embot::os::InitThread::releaseresources()
//{
//    pImpl->releaseresources();
//}

//bool embot::os::InitThread::resourcesarereleased() const
//{
//    
//}


// - class IdleThread

// --------------------------------------------------------------------------------------------------------------------
// - pimpl: private implementation (see scott meyers: item 22 of effective modern c++, item 31 of effective c++
// --------------------------------------------------------------------------------------------------------------------

struct embot::os::IdleThread::Impl
{    
    bool started {false};
    embot::os::rtos::thread_t *rtosthread {nullptr};
    //Config config {};
        
    Impl() = default; 
    
    ~Impl() { }     
};


// --------------------------------------------------------------------------------------------------------------------
// - all the rest
// --------------------------------------------------------------------------------------------------------------------


embot::os::IdleThread& embot::os::IdleThread::getInstance()
{
    static IdleThread* p = new IdleThread();
    return *p;
}

embot::os::IdleThread::IdleThread()
{   
    pImpl = std::make_unique<Impl>();    
}


embot::os::IdleThread::~IdleThread() { }


embot::os::Thread::Type embot::os::IdleThread::getType() const 
{
    return Type::Idle;
}


embot::os::Priority embot::os::IdleThread::getPriority() const
{   
    return embot::os::Priority::schedIdle;
}


bool embot::os::IdleThread::setPriority(embot::os::Priority priority)
{
    return false;
}

const char * embot::os::IdleThread::getName() const
{
   return "tIDLE"; 
}
  
bool embot::os::IdleThread::setEvent(embot::os::Event event)
{
    return embot::os::rtos::event_set(pImpl->rtosthread, event);
}  


bool embot::os::IdleThread::setMessage(embot::os::Message message, core::relTime timeout)
{
    return false;
}

bool embot::os::IdleThread::setValue(embot::os::Value value, core::relTime timeout)
{
    return false;
}

bool embot::os::IdleThread::setCallback(const core::Callback &callback, core::relTime timeout)
{
    return false;
}

void embot::os::IdleThread::synch()
{     
    pImpl->rtosthread = embot::os::rtos::scheduler_thread_running();
    embot::os::rtos::scheduler_associate(pImpl->rtosthread, this);
    pImpl->started = true;
}

void embot::os::IdleThread::run() {}

    
// - class EventThread

// --------------------------------------------------------------------------------------------------------------------
// - pimpl: private implementation (see scott meyers: item 22 of effective modern c++, item 31 of effective c++
// --------------------------------------------------------------------------------------------------------------------

struct embot::os::EventThread::Impl
{    
    EventThread * parentThread {nullptr};
    embot::os::rtos::thread_t *rtosthread {nullptr};
    Config config {64, Priority::minimum, nullptr, nullptr, embot::core::reltimeWaitForever, dummyOnEvent, "evtThread"};
    embot::os::rtos::thread_props_t rtosthreadproperties {};
    
    Impl(EventThread *parent) 
    {
        parentThread = parent;
        rtosthread = nullptr;        
    }
    
    ~Impl()
    {
        if(nullptr != rtosthread)
        {
            embot::os::rtos::scheduler_deassociate(rtosthread, parentThread);
            embot::os::rtos::thread_delete(rtosthread); 
            rtosthread = nullptr;
        }
    }
    
    static void dummyOnEvent(Thread *t, os::EventMask m, void *p) {}
              
                       
    static void os_eventdriven_loop(void *p) 
    {
        EventThread *t = reinterpret_cast<EventThread*>(p);
        const embot::core::relTime tout = t->pImpl->config.timeout; 
        const Thread::fpStartup startup = t->pImpl->config.startup;
        const Thread::fpOnEvent onevent = t->pImpl->config.onevent;
        void * param = t->pImpl->config.param;

        // exec the startup
        if(nullptr != startup)
        {
            startup(t, param);
        }
        
        // start the forever loop
        for(;;)
        {
            onevent(t, embot::os::rtos::event_get(tout), param);            
        }        
    }
    
};


// --------------------------------------------------------------------------------------------------------------------
// - all the rest
// --------------------------------------------------------------------------------------------------------------------


embot::os::EventThread::EventThread()
: pImpl(new Impl(this))
{   
    
}


embot::os::EventThread::~EventThread()
{   
    delete pImpl;
}


embot::os::Thread::Type embot::os::EventThread::getType() const 
{
    return Type::eventTrigger;
}


embot::os::Priority embot::os::EventThread::getPriority() const
{   
    return pImpl->config.priority;
}


bool embot::os::EventThread::setPriority(embot::os::Priority priority)
{
    pImpl->config.priority = priority;
    pImpl->rtosthreadproperties.setprio(embot::core::tointegral(priority));

    return embot::os::rtos::thread_setpriority(pImpl->rtosthread, priority);    
}

const char * embot::os::EventThread::getName() const
{
    return (pImpl->config.name != nullptr) ? pImpl->config.name : "EventThread"; 
}
  
bool embot::os::EventThread::setEvent(embot::os::Event event)
{
    return embot::os::rtos::event_set(pImpl->rtosthread, event); 
}  


bool embot::os::EventThread::setMessage(embot::os::Message message, core::relTime timeout)
{
    return false;
}

bool embot::os::EventThread::setValue(embot::os::Value value, core::relTime timeout)
{
    return false;
}

bool embot::os::EventThread::setCallback(const core::Callback &callback, core::relTime timeout)
{
    return false;
}

bool embot::os::EventThread::start(const Config &cfg, embot::core::fpCaller eviewername)
{    
    if(false == cfg.isvalid())
    {
        return false;
    }
    
    pImpl->config = cfg;
    
    pImpl->config.stacksize = (pImpl->config.stacksize+7)/8;
    pImpl->config.stacksize *= 8;

    pImpl->rtosthreadproperties.prepare((nullptr != eviewername) ? eviewername : pImpl->os_eventdriven_loop, 
                                        this, 
                                        embot::core::tointegral(pImpl->config.priority), 
                                        pImpl->config.stacksize);  
        
    pImpl->rtosthread = embot::os::rtos::thread_new(pImpl->rtosthreadproperties);
    embot::os::rtos::scheduler_associate(pImpl->rtosthread, this);   
    
    return true;    
}

void embot::os::EventThread::run()
{
    pImpl->os_eventdriven_loop(this);
}


// - class MessageThread

// --------------------------------------------------------------------------------------------------------------------
// - pimpl: private implementation (see scott meyers: item 22 of effective modern c++, item 31 of effective c++
// --------------------------------------------------------------------------------------------------------------------

struct embot::os::MessageThread::Impl
{    
    MessageThread * parentThread {nullptr};
    embot::os::rtos::thread_t *rtosthread {nullptr};    
    embot::os::rtos::messagequeue_t *osmessagequeue {nullptr};    
    Config config {64, Priority::minimum, nullptr, nullptr, embot::core::reltimeWaitForever, 2, dummyOnMessage, "msgThread"};
    embot::os::rtos::thread_props_t rtosthreadproperties {};
    
    Impl(MessageThread *parent) 
    {
        parentThread = parent;
        rtosthread = nullptr;
        osmessagequeue = nullptr;
    }
    
    ~Impl()
    {
        if(nullptr != rtosthread)
        {
            embot::os::rtos::scheduler_deassociate(rtosthread, parentThread);
            embot::os::rtos::thread_delete(rtosthread);     
            rtosthread = nullptr;
        }
        
        if(nullptr != osmessagequeue)
        {
            embot::os::rtos::messagequeue_delete(osmessagequeue);
            osmessagequeue = nullptr;
        }
    }
    
    static void dummyOnMessage(Thread *t, os::Message m, void *p) {}
        
    
    static void os_messagedriven_loop(void *p) 
    {
        MessageThread *t = reinterpret_cast<MessageThread*>(p);
        const embot::core::relTime tout = t->pImpl->config.timeout; 
        const Thread::fpStartup startup = t->pImpl->config.startup;
        const Thread::fpOnMessage onmessage = t->pImpl->config.onmessage;
        void * param = t->pImpl->config.param;
        embot::os::rtos::messagequeue_t *mq = t->pImpl->osmessagequeue;
       

        // exec the startup
        if(nullptr != startup)
        {
            startup(t, param);
        }

        
        // start the forever loop
        for(;;)
        {
            os::Message msg = embot::os::rtos::messagequeue_get(mq, tout);
            onmessage(t, msg, param);
        }        
    }
    
};



// --------------------------------------------------------------------------------------------------------------------
// - all the rest
// --------------------------------------------------------------------------------------------------------------------

embot::os::MessageThread::MessageThread()
: pImpl(new Impl(this))
{   
    
}


embot::os::MessageThread::~MessageThread()
{   
    delete pImpl;
}


embot::os::Thread::Type embot::os::MessageThread::getType() const
{
    return Type::messageTrigger;
}


embot::os::Priority embot::os::MessageThread::getPriority() const
{   
    return pImpl->config.priority;
}


bool embot::os::MessageThread::setPriority(embot::os::Priority priority)
{   
    pImpl->config.priority = priority;
    pImpl->rtosthreadproperties.setprio(embot::core::tointegral(priority));

    return embot::os::rtos::thread_setpriority(pImpl->rtosthread, priority);
}

const char * embot::os::MessageThread::getName() const
{
    return (pImpl->config.name != nullptr) ? pImpl->config.name : "MessageThread"; 
}
  
bool embot::os::MessageThread::setEvent(embot::os::Event event)
{
    return false;
}  

bool embot::os::MessageThread::setMessage(embot::os::Message message, core::relTime timeout)
{
    return embot::os::rtos::messagequeue_put(pImpl->osmessagequeue, message, timeout);
}

bool embot::os::MessageThread::setValue(embot::os::Value value, core::relTime timeout)
{
    return false;
}

bool embot::os::MessageThread::setCallback(const core::Callback &callback, core::relTime timeout)
{
    return false;
}


bool embot::os::MessageThread::start(const Config &cfg, embot::core::fpCaller eviewername)
{    
    if(false == cfg.isvalid())
    {
        return false;
    }
    
    pImpl->config = cfg;
    
    pImpl->config.stacksize = (pImpl->config.stacksize+7)/8;
    pImpl->config.stacksize *= 8;
    
    pImpl->osmessagequeue = embot::os::rtos::messagequeue_new(pImpl->config.messagequeuesize); 
    
    pImpl->rtosthreadproperties.prepare((nullptr != eviewername) ? eviewername : pImpl->os_messagedriven_loop, 
                                        this, 
                                        embot::core::tointegral(pImpl->config.priority), 
                                        pImpl->config.stacksize);
       
    pImpl->rtosthread = embot::os::rtos::thread_new(pImpl->rtosthreadproperties);
    
    embot::os::rtos::scheduler_associate(pImpl->rtosthread, this);
    
    return true;    
}


void embot::os::MessageThread::run()
{
    pImpl->os_messagedriven_loop(this);
}



// - class ValueThread

// --------------------------------------------------------------------------------------------------------------------
// - pimpl: private implementation (see scott meyers: item 22 of effective modern c++, item 31 of effective c++
// --------------------------------------------------------------------------------------------------------------------

struct embot::os::ValueThread::Impl
{    
    ValueThread * parentThread {nullptr};
    embot::os::rtos::thread_t *rtosthread {nullptr};    
    embot::os::rtos::messagequeue_t *osmessagequeue {nullptr};    
    Config config {64, Priority::minimum, nullptr, nullptr, embot::core::reltimeWaitForever, 2, dummyOnValue, "valThread"};
    embot::os::rtos::thread_props_t rtosthreadproperties {};
    
    Impl(ValueThread *parent) 
    {
        parentThread = parent;
        rtosthread = nullptr;
        osmessagequeue = nullptr;
    }
    
    ~Impl()
    {
        if(nullptr != rtosthread)
        {
            embot::os::rtos::scheduler_deassociate(rtosthread, parentThread);
            embot::os::rtos::thread_delete(rtosthread);     
            rtosthread = nullptr;
        }
        
        if(nullptr != osmessagequeue)
        {
            embot::os::rtos::messagequeue_delete(osmessagequeue);
            osmessagequeue = nullptr;
        }
    }
    
    static void dummyOnValue(Thread *t, os::Value v, void *p) {}
           
    static void os_valuedriven_loop(void *p) 
    {
        ValueThread *t = reinterpret_cast<ValueThread*>(p);
        const embot::core::relTime tout = t->pImpl->config.timeout; 
        const Thread::fpStartup startup = t->pImpl->config.startup;
        const Thread::fpOnValue onvalue = t->pImpl->config.onvalue;
        void * param = t->pImpl->config.param;
        embot::os::rtos::messagequeue_t *mq = t->pImpl->osmessagequeue;
       

        // exec the startup
        if(nullptr != startup)
        {
            startup(t, param);
        }

        
        // start the forever loop
        for(;;)
        {
            os::Message msg = embot::os::rtos::messagequeue_get(mq, tout);
            onvalue(t, reinterpret_cast<os::Value>(msg), param);
        }        
    }
    
};



// --------------------------------------------------------------------------------------------------------------------
// - all the rest
// --------------------------------------------------------------------------------------------------------------------

embot::os::ValueThread::ValueThread()
: pImpl(new Impl(this))
{   
    
}


embot::os::ValueThread::~ValueThread()
{   
    delete pImpl;
}


embot::os::Thread::Type embot::os::ValueThread::getType() const
{
    return Type::valueTrigger;
}


embot::os::Priority embot::os::ValueThread::getPriority() const
{   
    return pImpl->config.priority;
}


bool embot::os::ValueThread::setPriority(embot::os::Priority priority)
{   
    pImpl->config.priority = priority;
    pImpl->rtosthreadproperties.setprio(embot::core::tointegral(priority));

    return embot::os::rtos::thread_setpriority(pImpl->rtosthread, priority);
}

const char * embot::os::ValueThread::getName() const
{
    return (pImpl->config.name != nullptr) ? pImpl->config.name : "ValueThread"; 
}
  
bool embot::os::ValueThread::setEvent(embot::os::Event event)
{
    return false;
}  

bool embot::os::ValueThread::setMessage(embot::os::Message message, core::relTime timeout)
{
    return false;
}

bool embot::os::ValueThread::setValue(embot::os::Value value, core::relTime timeout)
{
    return embot::os::rtos::messagequeue_put(pImpl->osmessagequeue, reinterpret_cast<embot::os::Message>(value), timeout);
}

bool embot::os::ValueThread::setCallback(const core::Callback &callback, core::relTime timeout)
{
    return false;
}


bool embot::os::ValueThread::start(const Config &cfg, embot::core::fpCaller eviewername)
{    
    if(false == cfg.isvalid())
    {
        return false;
    }
    
    pImpl->config = cfg;
    
    pImpl->config.stacksize = (pImpl->config.stacksize+7)/8;
    pImpl->config.stacksize *= 8;
    
    pImpl->osmessagequeue = embot::os::rtos::messagequeue_new(pImpl->config.messagequeuesize); 
    
    pImpl->rtosthreadproperties.prepare((nullptr != eviewername) ? eviewername : pImpl->os_valuedriven_loop, 
                                        this, 
                                        embot::core::tointegral(pImpl->config.priority), 
                                        pImpl->config.stacksize);
       
    pImpl->rtosthread = embot::os::rtos::thread_new(pImpl->rtosthreadproperties);
    
    embot::os::rtos::scheduler_associate(pImpl->rtosthread, this);
    
    return true;    
}


void embot::os::ValueThread::run()
{
    pImpl->os_valuedriven_loop(this);
}


// - class CallbackThread

// --------------------------------------------------------------------------------------------------------------------
// - pimpl: private implementation (see scott meyers: item 22 of effective modern c++, item 31 of effective c++
// --------------------------------------------------------------------------------------------------------------------

struct embot::os::CallbackThread::Impl
{    
    CallbackThread * parentThread {nullptr};
    embot::os::rtos::thread_t *rtosthread {nullptr};    
    embot::os::rtos::messagequeue_t *osfunctionqueue {nullptr};
    embot::os::rtos::messagequeue_t *osargumentqueue {nullptr};
    Config config {64, Priority::minimum, nullptr, nullptr, embot::core::reltimeWaitForever, 2, dummyAfter, "cbkThread"};    
    embot::os::rtos::thread_props_t rtosthreadproperties {};
    
    Impl(CallbackThread *parent) 
    {
        parentThread = parent;
        rtosthread = nullptr;
        osfunctionqueue = nullptr;
        osargumentqueue = nullptr;  
    }
    
    ~Impl()
    {
        if(nullptr != rtosthread)
        {
            embot::os::rtos::scheduler_deassociate(rtosthread, parentThread);
            embot::os::rtos::thread_delete(rtosthread);     
            rtosthread = nullptr;
        }
        
        if(nullptr != osfunctionqueue)
        {
            embot::os::rtos::messagequeue_delete(osfunctionqueue);
            osfunctionqueue = nullptr;
        }
        
        if(nullptr != osargumentqueue)
        {
            embot::os::rtos::messagequeue_delete(osargumentqueue);
            osargumentqueue = nullptr;
        }
    }
    
    static void dummyAfter(Thread *t, core::Callback &m, void *p) {}
      
    // embot::os::rtos::messagequeue_get() returns a osal_message_t which is a uint32_t* which may hold a simple integer or a pointer to larger data
    // in case of os_get_caller() the content is a pointer to function        
    static embot::core::fpCaller os_get_caller(embot::os::rtos::messagequeue_t *mq,  embot::core::relTime tout)
    {
        return reinterpret_cast<embot::core::fpCaller>(embot::os::rtos::messagequeue_get(mq, tout));
    }
    
    // in case of os_get_argument() the content is a pointer to void
    static void* os_get_argument(embot::os::rtos::messagequeue_t *mq,  embot::core::relTime tout)
    {
        return reinterpret_cast<void*>(embot::os::rtos::messagequeue_get(mq, tout));
    }

    static void os_callbackdriven_loop(void *p) 
    {
        CallbackThread *t = reinterpret_cast<CallbackThread*>(p);
        const embot::core::relTime tout = t->pImpl->config.timeout; 
        const Thread::fpStartup startup = t->pImpl->config.startup;
        const Thread::fpAfterCallback after = (nullptr != t->pImpl->config.aftercallback) ? (t->pImpl->config.aftercallback) : (dummyAfter);
        void * param = t->pImpl->config.param;
        embot::os::rtos::messagequeue_t *fQ = t->pImpl->osfunctionqueue;
        embot::os::rtos::messagequeue_t *aQ = t->pImpl->osargumentqueue;
        
        embot::core::Callback cbk;;
       

        // exec the startup
        if(nullptr != startup)
        {
            startup(t, param);
        }
        
        // start the forever loop
        for(;;)
        {
            // the order is important in here.... dont exchange the following two lines .....
            cbk.arg = os_get_argument(aQ, tout); 
            cbk.call = os_get_caller(fQ, tout);
            // it executes only if it is valid 
            cbk.execute();
            after(t, cbk, param);
        }   

    }
    
};



// --------------------------------------------------------------------------------------------------------------------
// - all the rest
// --------------------------------------------------------------------------------------------------------------------


embot::os::CallbackThread::CallbackThread()
: pImpl(new Impl(this))
{   
    
}


embot::os::CallbackThread::~CallbackThread()
{   
    delete pImpl;
}


embot::os::Thread::Type embot::os::CallbackThread::getType() const
{
    return Type::callbackTrigger;
}


embot::os::Priority embot::os::CallbackThread::getPriority() const
{   
    return pImpl->config.priority;
}


bool embot::os::CallbackThread::setPriority(embot::os::Priority priority)
{
    pImpl->config.priority = priority;
    pImpl->rtosthreadproperties.setprio(embot::core::tointegral(priority));

    return embot::os::rtos::thread_setpriority(pImpl->rtosthread, priority); 
}

const char * embot::os::CallbackThread::getName() const
{
    return (pImpl->config.name != nullptr) ? pImpl->config.name : "CallbackThread"; 
}
  
bool embot::os::CallbackThread::setEvent(embot::os::Event event)
{
    return false;
}  


bool embot::os::CallbackThread::setMessage(embot::os::Message message, core::relTime timeout)
{
    return false;
}

bool embot::os::CallbackThread::setValue(embot::os::Value value, core::relTime timeout)
{
    return false;
}

bool embot::os::CallbackThread::setCallback(const embot::core::Callback &callback, core::relTime timeout)
{
    if(false == callback.isvalid())
    {
        return false;
    }
    
    if(true == embot::os::rtos::messagequeue_put(pImpl->osargumentqueue, reinterpret_cast<embot::os::Message>(callback.arg), timeout))
    {
        return embot::os::rtos::messagequeue_put(pImpl->osfunctionqueue, reinterpret_cast<embot::os::Message>(callback.call), timeout);
    }
    
    return false;
}

bool embot::os::CallbackThread::start(const Config &cfg, embot::core::fpCaller eviewername)
{    
    if(false == cfg.isvalid())
    {
        return false;
    }
    
    pImpl->config = cfg;    
    
    pImpl->config.stacksize = (pImpl->config.stacksize+7)/8;
    pImpl->config.stacksize *= 8;
    
    pImpl->osargumentqueue = embot::os::rtos::messagequeue_new(pImpl->config.queuesize); 
    pImpl->osfunctionqueue = embot::os::rtos::messagequeue_new(pImpl->config.queuesize); 
    
    pImpl->rtosthreadproperties.prepare((nullptr != eviewername) ? eviewername : pImpl->os_callbackdriven_loop, 
                                        this, 
                                        embot::core::tointegral(pImpl->config.priority), 
                                        pImpl->config.stacksize);
       
    pImpl->rtosthread = embot::os::rtos::thread_new(pImpl->rtosthreadproperties);    
    embot::os::rtos::scheduler_associate(pImpl->rtosthread, this);
    
    return true;    
}


void embot::os::CallbackThread::run()
{
    pImpl->os_callbackdriven_loop(this);
}


// - class PeriodicThread

// --------------------------------------------------------------------------------------------------------------------
// - pimpl: private implementation (see scott meyers: item 22 of effective modern c++, item 31 of effective c++
// --------------------------------------------------------------------------------------------------------------------

struct embot::os::PeriodicThread::Impl
{    
    PeriodicThread * parentThread {nullptr};
    embot::os::rtos::thread_t *rtosthread {nullptr};    
    Config config {64, Priority::minimum, nullptr, nullptr, embot::core::time1second, dummyOnPeriod, "perThread"};    
    embot::os::rtos::thread_props_t rtosthreadproperties {};
    
    Impl(PeriodicThread *parent) 
    {
        parentThread = parent;
        rtosthread = nullptr;
    }
    
    ~Impl()
    {
        if(nullptr != rtosthread)
        {
            embot::os::rtos::thread_period_set(rtosthread, 0);
            embot::os::rtos::scheduler_deassociate(rtosthread, parentThread);
            embot::os::rtos::thread_delete(rtosthread);     
            rtosthread = nullptr;
        }        
    }
    
    static void dummyOnPeriod(Thread *t, void *p) {}
    
    static void os_periodic_loop(void *p) 
    {
        PeriodicThread *t = reinterpret_cast<PeriodicThread*>(p);
        const embot::core::relTime period = t->pImpl->config.period; 
        const Thread::fpStartup startup = t->pImpl->config.startup;
        const Thread::fpOnPeriod onperiod = t->pImpl->config.onperiod;
        void * param = t->pImpl->config.param;
       

        // exec the startup
        if(nullptr != startup)
        {
            startup(t, param);
        }

        embot::os::rtos::thread_period_set(t->pImpl->rtosthread, period);
        
        // start the forever loop
        for(;;)
        {
            embot::os::rtos::thread_period_wait(t->pImpl->rtosthread, period);
            onperiod(t, param);
        }        
    }
    
};


// --------------------------------------------------------------------------------------------------------------------
// - all the rest
// --------------------------------------------------------------------------------------------------------------------


embot::os::PeriodicThread::PeriodicThread()
: pImpl(new Impl(this))
{   
    
}


embot::os::PeriodicThread::~PeriodicThread()
{   
    delete pImpl;
}


embot::os::Thread::Type embot::os::PeriodicThread::getType() const
{
    return Type::periodicTrigger;
}


embot::os::Priority embot::os::PeriodicThread::getPriority() const
{   
    return pImpl->config.priority;
}


bool embot::os::PeriodicThread::setPriority(embot::os::Priority priority)
{
    pImpl->config.priority = priority;
    pImpl->rtosthreadproperties.setprio(embot::core::tointegral(priority));

    return embot::os::rtos::thread_setpriority(pImpl->rtosthread, priority); 
}

const char * embot::os::PeriodicThread::getName() const
{
    return (pImpl->config.name != nullptr) ? pImpl->config.name : "PeriodicThread"; 
}

  
bool embot::os::PeriodicThread::setEvent(embot::os::Event event)
{
    return false;
}  


bool embot::os::PeriodicThread::setMessage(embot::os::Message message, core::relTime timeout)
{
    return false;
}

bool embot::os::PeriodicThread::setCallback(const core::Callback &callback, core::relTime timeout)
{
    return false;
}

bool embot::os::PeriodicThread::setValue(embot::os::Value value, core::relTime timeout)
{
    return false;
}

bool embot::os::PeriodicThread::start(const Config &cfg, embot::core::fpCaller eviewername)
{    
    if(false == cfg.isvalid())
    {
        return false;
    }
    
    pImpl->config = cfg;
    
    pImpl->config.stacksize = (pImpl->config.stacksize+7)/8;
    pImpl->config.stacksize *= 8;
    
    pImpl->rtosthreadproperties.prepare((nullptr != eviewername) ? eviewername : pImpl->os_periodic_loop, 
                                        this, 
                                        embot::core::tointegral(pImpl->config.priority), 
                                        pImpl->config.stacksize);
           
    pImpl->rtosthread = embot::os::rtos::thread_new(pImpl->rtosthreadproperties);    
    embot::os::rtos::scheduler_associate(pImpl->rtosthread, this);
    
    return true;    
}


void embot::os::PeriodicThread::run()
{
    pImpl->os_periodic_loop(this);
}

    
// - end-of-file (leave a blank line after)----------------------------------------------------------------------------

