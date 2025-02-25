
/*
 * Copyright (C) 2020 iCub Tech - Istituto Italiano di Tecnologia
 * Author:  Marco Accame
 * email:   marco.accame@iit.it
*/


// - include guard ----------------------------------------------------------------------------------------------------

#ifndef _EMBOT_APP_APPLICATION_THEPOSREADER2_H_
#define _EMBOT_APP_APPLICATION_THEPOSREADER2_H_


#include "embot_app_application_theCANparserPOS.h"

#include "embot_core.h"
#include "embot_hw.h"
#include "embot_os.h"
#include "embot_os_Thread.h"
#include "embot_prot_can.h"
#include "embot_prot_can_analog_polling.h"
#include "embot_prot_can_analog_periodic.h"
#include "embot_hw_types.h"
#include <array>
#include <memory>

namespace embot { namespace app { namespace application {
           
    class thePOSreader2 : public CANagentPOS
    {
    public:
        static thePOSreader2& getInstance();
    
        // the measurement units is:
        // centideg = 0.01 degrees for sensorType::tlv and sensorType::lr17
        // micrometer = 0.001 mm for sensorType::qe
        using Position = int32_t; 
    
        static constexpr Position valueOfPositionACQUISITIONnotvalid {1000*100}; // 1000 degrees or 
        
        static constexpr std::uint8_t numberofpositions = 9; // if 7 ... upto the lr17. if 9 ... also the two qe
    
        enum class sensorType { tlv = 0, lr17 = 1, qe = 2, none = 255 }; 
        
        struct Sensor
        {
            sensorType type {sensorType::none};
            embot::hw::ANY id {embot::hw::ANY::none};
            embot::prot::can::analog::posLABEL label {embot::prot::can::analog::posLABEL::zero};
            embot::os::Event askdata {0};           // used to ask the thread to begin acquisition from the sensor (in non-blocking mode)
            embot::os::Event dataready {0};         // used by the hw to alert the thread that the data is available
            embot::os::Event noreply {0};           // SO FAR NOT USED: maybe used to alert the waiting thread that there is no reply from the sensor after the specified timeout
            embot::core::relTime timeout {0};       // SO FAR NOT USED: the time allowed for this sensor before we can emit the noreply event. 
            constexpr Sensor() = default;
            constexpr Sensor(sensorType ty, embot::hw::ANY i, embot::prot::can::analog::posLABEL la, embot::os::Event as, embot::os::Event dr,  embot::os::Event nr = 0, embot::core::relTime to = 0) 
                    : type(ty), id(i), label(la), askdata(as), dataready(dr), noreply(nr), timeout(to)  {}
            constexpr bool isvalid() const { return type != sensorType::none; }
        };
                   
        
        // a timer initted by thePOSreader2 emits the event transmit in regular mode as commanded by Message_POS_TRANSMIT
        // event askdata[s] starts acquisition of the s-th sensor 
        // event dataready[s] tells that s-th sensor has finished acquisition. it follows askdata[s] unless the chip is broken, in such a case
        // event timeout[s] may be used to tell that dataready[s] has not arrived ... BOH.
        // the way askdata[s] is emitted by thePOSreader2 ... depends on the parallelism of the chips
        // if all are independent, thePOSreader2 may emit all the askdata[] events at the same time.
        // if they all are on the same bus, it will emit one after another. 
        // if as in the pmc board they share a bus two by two, then thePOSreader2 can start acquitions in two times
        // and what about the acquisition frequency? well that is not decided yet.                         
        struct Events
        {
            embot::os::Event acquire {0};
            embot::os::Event noreply {0}; // atleastonereplyismissing
            embot::os::Event transmit {0};           
            constexpr Events() = default;
            constexpr Events(embot::os::Event ac, embot::os::Event nr, embot::os::Event tx) : acquire(ac), noreply(nr), transmit(tx) {}            
            constexpr bool isvalid() const { if((0 == acquire) || (0 == transmit)) { return false; } return true; }
        }; 
        
        // so far only AcquisitionMode::daisychain is supported
        enum class AcquisitionMode {    // at every reception of the Events::acquire event:
            daisychain,                 // foreach(valid sensor){ <emit the Sensor::askdata event> and <wait for the Sensor::dataready> } 
            fullyparallel,              // emit at the same time the events Sensor::askdata of every valid sensor and wait for all of their Sensor::dataready events 
            mod2parallel                // acquire in parallel at first the valid sensors in even positions and then all those in odd positions
        };
        
        // TODO: move from a std::array<> to a std::vector<> so that we can better manage the number of sensors. 
        // even if we cannot manage more than ... 10 because of the fact the we can mange max 32 events per thread and the 
        // reader thread must accept: three events for each sensor (askdata, dataready, noreply) + Events::acquire        
        struct Config
        {               
            AcquisitionMode acquisitionmode { AcquisitionMode::daisychain };
            embot::core::relTime acquisitionperiod {0}; // if 0: use the same period of transmission as ordered by Message_POS_TRANSMIT, else acquisition and transmission are unrelated.             
            embot::core::relTime acquisitiontimeout {0};
            embot::os::Thread* reader {nullptr};        // the thread which processes Events::acquire and every (Sensor::askdata, Sensor::dataready, Sensor::noreply)
            embot::os::Thread* transmitter {nullptr};   // the thread which processes Events::transmit emitted by thread reader or by a timer internal to thePOSreader2
            std::array<Sensor, numberofpositions> sensors {{}};
            Events events {};   
            constexpr Config() = default;
            constexpr Config(AcquisitionMode am, embot::core::relTime ap, embot::core::relTime at, embot::os::Thread* re, embot::os::Thread* tx, const std::array<Sensor, numberofpositions>& s, const Events& e) 
             : acquisitionmode(am), acquisitionperiod(ap), acquisitiontimeout(at), reader(re), transmitter(tx), sensors(s), events(e) {}             
            bool isvalid() const { return events.isvalid(); }
        }; 
        
        
        bool initialise(const Config &config);   
        
        bool start(embot::core::relTime period = 0);
        bool stop(); 
        
        bool isvalid(embot::os::EventMask evtmask) const;
        
        // this is called by the reader thread when it receives an event mask.        
        bool process(embot::os::EventMask evtmask);
        
        // only the transmitter must call it to retrieve the frames 
        bool get(std::vector<embot::prot::can::Frame> &frames2transmit);
       
        
        // interface to CANagentPOS
        virtual bool set(const embot::prot::can::analog::polling::Message_POS_CONFIG_SET::Info &info);
        virtual bool set(const embot::prot::can::analog::polling::Message_POS_TRANSMIT::Info &info);
        virtual bool get(const embot::prot::can::analog::polling::Message_POS_CONFIG_GET::Info &info, embot::prot::can::analog::polling::Message_POS_CONFIG_GET::ReplyInfo &replyinfo);
        

    private:
        thePOSreader2(); 
        ~thePOSreader2(); 

    private:    
        struct Impl;
        std::unique_ptr<Impl> pImpl;    
    };       


}}} // namespace embot { namespace app { namespace application


#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------
