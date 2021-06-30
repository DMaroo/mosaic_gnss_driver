#ifndef MOSAIC_GNSS_DRIVER_DATA_BUFFERS_H
#define MOSAIC_GNSS_DRIVER_DATA_BUFFERS_H

// #include <mutex>

// Required Message Types
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TwistWithCovarianceStamped.h>
#include <nmea_msgs/Gpgga.h>
#include <nmea_msgs/Sentence.h>
#include <sensor_msgs/NavSatFix.h>
#include <sensor_msgs/TimeReference.h>

#ifdef __JETBRAINS_IDE__ // remove flag to get ide hints on entire class
#undef MOSAIC_GNSS_CORE_ONLY
#endif

// MOSAIC_GNSS_CORE_ONLY should be defined if we are compiling the core library.
// this ignores the ros parts of the data buffers, i.e. only exposes the ptr_t, get_new_ptr, and
// set_ptr. Never instantiate a member of these classes in the core library. #ifndef
// MOSAIC_GNSS_CORE_ONLY

#include <ros/publisher.h>
#include <ros/ros.h>

// #endif

namespace mosaic_gnss_driver {
    template <typename msg_type>
    struct Buffer
    {
    public:
        using ptr_t = std::unique_ptr<msg_type>;

        Buffer() : initPub(false) {}

    private:
        // std::mutex mutex; // NOTE: We need a mutex if the publishers run on another thread
        ptr_t ptr{nullptr}, old{nullptr};

    public:
        bool enabled, initPub;

        /**
         * Returns a pointer to an instance of the message. This should be filled and passed to
         * set_ptr Not guaranteed to be zero initialized, i.e. implementation may reuse previous
         * unsent message.
         *
         * @return message pointer
         */
        ptr_t get_new_ptr()
        {
            // Reuse old ptr if it wasn't sent yet
            if (old)
                return std::move(old);
            else
                return std::make_unique<msg_type>();
        }

        /**
         * Sets the message
         * @param new_ptr filled in message, consumed, should be generated by get_new_ptr only
         */
        void set_ptr(ptr_t new_ptr)
        {
            if (!enabled)
                return;
            old = std::move(new_ptr);
            ptr.swap(old);
        }

        /**
         * Get the stored message.
         * After this the buffer will be empty.
         *
         * @return message, may be nullptr
         */
        ptr_t get() { return std::move(ptr); }

    private:
        ros::Publisher pub;
        // We do this to compile core library without ros.
        // The core library never creates an object of this type, it only calls the above
        // functions.`

#ifndef MOSAIC_GNSS_CORE_ONLY
    public:
        // init must be called before calling publish

        void init(ros::NodeHandle& nh, const char* topic, size_t queue, bool latch = false)
        {
            pub = nh.advertise<msg_type>(topic, queue, latch);
            initPub = true;
        }

        void publish()
        {
            // std::lock_guard<std::mutex> lock(mutex);
            if (!enabled || !initPub)
                return;
            if (!ptr)
            {
                // #include <boost/core/demangle.hpp>

                // ROS_WARN("Not enough msg %s",
                // boost::core::demangle(typeid(msg_type).name()).data());
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
        Buffer<nmea_msgs::Sentence> nmea_sentence;
        Buffer<sensor_msgs::TimeReference> time_reference;

#ifndef MOSAIC_GNSS_CORE_ONLY

        void publish_all()
        {
            nav_sat_fix.publish();
            pose.publish();
            velocity.publish();
            nmea_sentence.publish();
            time_reference.publish();
        }

#endif
    };
} // namespace mosaic_gnss_driver

#endif // MOSAIC_GNSS_DRIVER_DATA_BUFFERS_H
