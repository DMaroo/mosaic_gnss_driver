#ifndef MOSAIC_GNSS_DRIVER_DATA_BUFFERS_H
#define MOSAIC_GNSS_DRIVER_DATA_BUFFERS_H

// #include <mutex>

// Required Message Types
#include <sensor_msgs/NavSatFix.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>

#ifdef __JETBRAINS_IDE__ // remove flag to get ide hints on entire class
#undef MOSAIC_GNSS_CORE_ONLY
#endif

// MOSAIC_GNSS_CORE_ONLY should be defined if we are compiling the core library.
// this ignores the ros parts of the data buffers, i.e. only exposes the ptr_t, get_new_ptr, and set_ptr.
// Never instantiate a member of these classes in the core library.
// #ifndef MOSAIC_GNSS_CORE_ONLY

#include <ros/ros.h>
#include <ros/publisher.h>

// #endif

namespace mosaic_gnss_driver
{
    template<typename msg_type>
    struct Buffer
    {
    public:
        using ptr_t = std::unique_ptr<msg_type>;
    private:
        // std::mutex mutex; // NOTE: We need a mutex if the publishers run on another thread
        ptr_t ptr{nullptr};
    public:

        /**
         * Returns a pointer to an instance of the message. This should be filled and passed to set_ptr
         * Not guaranteed to be zero initialized, i.e. implementation may reuse previous unsent message.
         *
         * @return message pointer
         */
        ptr_t get_new_ptr()
        {
            // TODO: Reuse old ptr if not sent yet ?
            return std::make_unique<msg_type>();
        }

        /**
         * Sets the message
         * @param new_ptr filled in message, consumed, should be generated by get_new_ptr only
         */
        void set_ptr(ptr_t new_ptr)
        {
            ptr = std::move(new_ptr);
        }

        /**
         * Get the stored message.
         * After this the buffer will be empty.
         *
         * @return message, may be nullptr
         */
        ptr_t get()
        {
            return std::move(ptr);
        }

// We do this to compile core library without ros.
// The core library never creates an object of this type, it only calls the above functions.`
    private:
        ros::Publisher pub;
#ifndef MOSAIC_GNSS_CORE_ONLY
    public:
        // init must be called before calling publish

        void init(ros::NodeHandle &nh, const char *topic, size_t queue, bool latch = false)
        {
            pub = nh.advertise<msg_type>(topic, queue, latch);
        }


        void publish()
        {
            // std::lock_guard<std::mutex> lock(mutex);
            if (!ptr)
            {
#include <boost/core/demangle.hpp>
                ROS_WARN("Not enough msg %s", boost::core::demangle(typeid(msg_type).name()).data());
            } else
            {
                // TODO: Check if publisher ready
                typename msg_type::Ptr shared_ptr = std::move(ptr);
                pub.publish(shared_ptr);
            }
        }

#endif // MOSAIC_GNSS_CORE_ONLY
    };

    // Buffers for various messages
    // Object of this type should never be created in the core library, only accessed over reference
    struct DataBuffers
    {
        Buffer<sensor_msgs::NavSatFix> nav_sat_fix;
        Buffer<geometry_msgs::PoseWithCovarianceStamped> pose;
        Buffer<geometry_msgs::TwistWithCovarianceStamped> velocity;
    };
}

#endif //MOSAIC_GNSS_DRIVER_DATA_BUFFERS_H
