## Privaros - A Framework for Privacy Compliant Drones

Privaros is a framework developed to enforce privacy policies on drones, especially commercial delivery drones which potentially visit a number of host airspaces, each of which may have different privacy requirements. It allows host airspaces (e.g., a corporate or university campus, a city neighbourhood, or an apartment/condominium complex) to ensure that guest drones entering them are compliant with their privacy-policies. Privaros enhances the drone software stack with mechanisms that provide a mandatory access control framework to enforce host policies on guest drones. Privaros is tailored for ROS (a middleware popular in many drone platforms) and makes use of SROS and the Linux Security Module infrastructure to provide access control at both the application and operating system layers.

Kindly refer to our [paper](https://www.csa.iisc.ac.in/~vg/papers/ccs2020/ccs2020.pdf) for more information.

> **Disclaimer:** Privaros is a research prototype. It is strongly advised that Privaros not be deployed on real-world equipment. The authors will not be responsible for any damages that may ensue.

## Components

### ROS
[ROS](https://index.ros.org/doc/ros2/) (Robot Operating System) is a collection of libraries and tools that facilitate the development of robotics applications. ROS uses [DDS](https://www.omg.org/omg-dds-portal/) (Data Distribution Service) as the underlying infrastructure atop which it builds its publish-subscribe framework. Please visit the respective websites to learn more about ROS2 and DDS.

Privaros makes the following changes to ROS2:

* **Over-Provisioning of Topics**
    To support dynamic redirection of information flow for off-the-shelf ROS2 applications, Privaros internally allocates temporary topics (the current number is 100) for every application. However, these topics are not available for use by application programmers, as they have been added solely for the purpose of dynamic redirection.
        
* **Flowcontroller Topic**
    Privaros modifies the ROS2 C++ client library (rclcpp) such that every application which links against it subscribes to a ```flowcontroller``` topic by default. Trusted applications (described below) use ```flowcontroller``` to publish messages (as commands) that facilitate the dynamic redirection of information flow. Every ROS2 application receives these messages on account of having subscribed to ```flowcontroller```.
Process id in the message helps to determine which application should use the message to perform redirection.

* **Trusted Applications**
    Trusted Applications (TAs) in Privaros implement the core logic that controls the information flow across ROS2 applications. TAs are identified by setting a specific flag ```is_trusted``` as part of their AppArmor profile. Privaros' implementation makes use of a single TA which links against shared libraries (as opposed to developing multiple TAs). Each shared library supports a different kind of filter functionality (blurring images for instance). We currently support filter functionality for messages of type ```sensor_msgs_image```. Feel free to add support for other types :)
    

### AppArmor

[AppArmor](https://gitlab.com/apparmor/apparmor/-/wikis/home) is a [Linux Security Module](https://www.kernel.org/doc/html/latest/security/lsm.html) (LSM) that provides path-based [Mandatory Access Control](https://en.wikipedia.org/wiki/Mandatory_access_control)  (MAC) on Linux systems. Privaros extends AppArmor's user-space tools as well ass LSM hooks to specify and enforce host airspace policies within the kernel as follows:

1) User-Space Tools - We extended AppArmor's parser to allow the *specification* of new policies that hosts of restricted spaces may be interested in. To achieve this, it is required to store additional information with every application's AppArmor profile. Some of the key additions include:
 
    * The application binary's SHA1 hash (can be generated using OpenSSL's CLI) - specified using rule ```dte_domain```
    * A white-list of other applications with which communication is allowed - specified using rule ```dte_net_domain``` (a value of '\*' allows the application confers unrestricted communication within the system)
    * A white-list of external IP addresses with which communication is allowed-  specified using rule ```allow_ip``` (a value of 0.0.0.0 confers unrestricted network access)
    * A flag to indicate a *'trusted'* application - specified using rule ```is_trusted```

    Here is a snippet which illustrates our additions to AppArmor's policy language:
    
    ```
    dte_domain {BINARY_HASH},
    dte_net_domain {BINARY_HASH}, # own binary hash
    dte_net_domain {BINARY_HASH}, # different process' hash
    is_trusted false,
    allow_ip 0.0.0.0,
    ```
    
2) Kernel LSM - Our extensions to AppArmor's LSM facilitate the *enforcement* of host polices. To achieve this, we extend some of AppArmor's existing hooks as well as add implementations for a few new hooks (all of which are part of the LSM framework). A detailed description of the additions can be found in our [paper](https://www.csa.iisc.ac.in/~vg/papers/ccs2020/ccs2020.pdf) and are beyond the scope of this document.

## Install

Privaros was developed on an [Nvidia Jetson TX2](https://www.nvidia.com/en-us/autonomous-machines/embedded-systems/jetson-tx2/) development kit running  Linux for Tegra ([L4T v32.2.3](https://developer.nvidia.com/embedded/linux-tegra-r3223)). It is therefore strongly recommended that Privaros be deployed and tested only on platforms that meet the above criteria.

For the rest of this document, we assume that users have access to a Jetson TX2 and are able to flash custom software. Kindly refer to the [L4T docs](https://docs.nvidia.com/jetson/l4t/index.html) for details about flashing the TX2.

Before proceeding, clone this repository and export its path to environment variable ```$PRIVAROS_SRC```

```
git clone https://github.com/iisc-cssl/privaros.git
export PRIVAROS_SRC=$(readlink -f privaros)
```

### Linux Kernel

To flash Privaros' kernel on the TX2, apply the kernel patch as follows:
> It is assumed that all software dependencies needed to compile TX2's kernel have been met. For instance, cross-compiling the kernel on a non-Jetson system requires the [L4T Toolchain](https://docs.nvidia.com/jetson/l4t/index.html#page/Tegra%2520Linux%2520Driver%2520Package%2520Development%2520Guide%2Fxavier_toolchain.html%23). Kindly verify that all dependencies are met from the L4T docs before proceeding.

```
# Fetch L4T sources
wget https://developer.nvidia.com/embedded/r32-2-3_Release_v1.0/Sources/T186/public_sources.tbz2

# Decompress and extract the archive
tar -xjf public_sources.tbz2
cd public_sources/

# Decompress and extract the Linux kernel
tar -xjf kernel_src.tbz2
export LINUX_KERNEL=$(readlink -f kernel)

# Generate the default kernel configuration for Nvidia Tegra
cd $LINUX_KERNEL/kernel-4.9
make ARCH=arm64 O=$LINUX_KERNEL/build tegra_defconfig

# Copy Privaros' config file
cp $PRIVAROS_SRC/kernel_config/.config $LINUX_KERNEL/build/.config

# Apply the patch
cp $PRIVAROS_SRC/patches/kernel_patch.patch $LINUX_KERNEL
cd $LINUX_KERNEL
patch -p0 < kernel_patch.patch
```
Build and flash the patched kernel on the TX2.  More information about customizing the Linux kernel for the TX2 can be found in the [Kernel Customization](https://docs.nvidia.com/jetson/l4t/index.html#page/Tegra%2520Linux%2520Driver%2520Package%2520Development%2520Guide%2Fkernel_custom.html%23) section of the [L4T documentation](https://docs.nvidia.com/jetson/l4t/index.html).

### ROS

Privaros was developed using [ROS2 Dashing Diademata](https://index.ros.org/doc/ros2/Releases/Release-Dashing-Diademata/). Kindly follow the instructions to apply Privaros' ROS2 patches. 

* Download ROS2 Dashing's [source code](https://index.ros.org/doc/ros2/Installation/Dashing/Linux-Development-Setup/) and follow the instructions up to  (and including) [Install dependencies using rosdep](https://index.ros.org/doc/ros2/Installation/Dashing/Linux-Development-Setup/#install-dependencies-using-rosdep). The remaining steps will assume that the root of your ROS2 workspace is stored within an environment variable ```$ROS2_WS```. Therefore, please ensure that this environment variable is exported.
* Apply the patch for RCLCPP
    ```
    cp $PRIVAROS_SRC/patches/rclcpp.patch $ROS2_WS/src/ros2/
    cd $ROS2_WS/src/ros2/
    patch -p0 < rclcpp.patch
    ```
* Apply the patch for eProsima FastRTPS
    ```
    cp $PRIVAROS_SRC/patches/fastrtps.patch $ROS2_WS/src/eProsima
    cd $ROS2_WS/src/eProsima/
    patch -p0 < fastrtps.patch
    ```
* Build and Install ROS2 by following the instructions from [here](https://index.ros.org/doc/ros2/Installation/Dashing/Linux-Development-Setup/#build-the-code-in-the-workspace)
* Setup SROS by following these [instructions](https://github.com/ros2/sros2/blob/dashing/SROS2_Linux.md). 
* Generate profiles for ROS applications using the following SROS policy template:
    ```
    <?xml version="1.0" encoding="UTF-8"?>
    <policy version="0.1.0"
      xmlns:xi="http://www.w3.org/2001/XInclude">
      <profiles>
        <profile ns="/" node="camera">
          <xi:include href="common/node.xml"
            xpointer="xpointer(/profile/*)"/>
          <topics publish="ALLOW" >
            <topic>imageraw</topic>
          </topics>
        </profile>
        <profile ns="/" node="blurfilter">
          <xi:include href="common/node.xml"
            xpointer="xpointer(/profile/*)"/>
          <topics publish="ALLOW" >
            <topic>output</topic>
          </topics>
          <topics subscribe="ALLOW" >
            <topic>input</topic>
          </topics>
        </profile>
        <profile ns="/" node="showimage">
          <xi:include href="common/node.xml"
            xpointer="xpointer(/profile/*)"/>
          <topics subscribe="ALLOW" >
            <topic>imageraw</topic>
          </topics>
        </profile>
        <profile ns="/" node="colonel">
          <xi:include href="common/node.xml"
            xpointer="xpointer(/profile/*)"/>
          <topics publish="ALLOW" >
            <topic>flowcontroller</topic>
          </topics>
        </profile>
      </profiles>
    </policy>
    ```

### AppArmor User-Space Tools

AppArmor's [user-space tools](https://gitlab.com/apparmor/apparmor) contain various libraries and utilities. However, Privaros only modifies its parser. Here is a shell script that downloads AppArmor's source code, applies the patch and finally builds and installs the components needed by Privaros. If you encounter errors, kindly refer to AppArmor's [installation instructions](https://gitlab.com/apparmor/apparmor/blob/apparmor-2.13/README.md). 
```
# Fetch AppArmor 2.13.2 source code
wget https://launchpad.net/apparmor/2.13/2.13.2/+download/apparmor-2.13.2.tar.gz

# Decompress and extract the archive
tar -xvzf apparmor-2.13.2.tar.gz
export APPARMOR_SRC=$(readlink -f apparmor-2.13.2)

# Apply the patch
cp $PRIVAROS_SRC/patches/apparmor_patch.patch $(dirname "$APPARMOR_SRC")
cd $(dirname "$APPARMOR_SRC")
patch -p0 < apparmor_patch.patch

# Build and Install libapparmor
cd $APPARMOR_SRC/libraries/libapparmor
sh ./autogen.sh
sh ./configure --prefix=/usr --with-perl --with-python
make && make install

# Build and Install AppArmor's parser
cd $APPARMOR_SRC/parser
make && make install 
```


## Usage

### Running ROS Applications

If Privaros was successfully installed, the following shell commands will launch a ROS2 ```talker``` (which publishes messages on topic 'chatter') and ```listener``` (which subscribes to 'chatter' and receives messages from talker):

```
ros2 run demo_nodes_cpp talker
ros2 run demo_nodes_cpp listener
```
    
If you are unable to launch ```talker```  and ```listener```, here are a few suggestions that may help fix the problem:

* Source the ROS2 'local_setup' shell script (depending on your shell). This can be found in the 'install' directory of your ROS2 workspace. For instance, a ZSH user would use:

    ```source {ros2_ws}/install/local_setup.zsh```

* Ensure that SROS profiles for talker and listener have been created such that talker is able to publish to 'chatter' and listener is able to subscribe to 'chatter'. Kindly refer to the [SROS documentation](https://github.com/ros2/sros2/blob/dashing/SROS2_Linux.md) for detailed instructions.

* Ensure that the AppArmor profiles for talker and listener:

    * have the correct binary hash values (specified in the ```domain_name``` policy rule)
    * are able to communicate with each other i.e. their profiles must have the following rules:

| talker | listener |
| -------- | -------- |
| ```dte_net_domain <talker_binary_hash>```   | ```dte_net_domain <listener_binary_hash>``` |
| ```dte_net_domain <listener_binary_hash>``` | ```dte_net_domain <talker_binary_hash>```   |


### Dynamic Redirection

To demonstrate dynamic redirection, we have created an application ```colonel``` (whose source can be found in ```{ros2_ws}/src/ros2/demos/demo_nodes_cpp/src/topics/colonel.cpp```). To launch it, execute the following command:

```
ros2 run demo_nodes_cpp colonel "<action>, <pid>, <old_topic_name>, <new_topic_name>"
```
where:
* ```action``` is one of {pub, sub}
* ```pid``` refers to the process ID of a ROS2 applications for which a change is to be made
* ```old_topic_name``` refers to the old topic that the ROS2 application was publishing/subscribing to
* ```new_topic_name``` refers to the new topic that the ROS2 application will publish/subscribe to

Let's take the example of a ROS2 application with PID 1234, that publishes to topic 'image_raw'. To make this application publish to topic 'image_blurred', we could execute the following command:

```
ros2 run demo_nodes_cpp colonel "pub, 1234, image_raw, image_blurred"
```

An alternative method by which you can achieve dynamic redirection is by using the script ```ros2_topic_changer.sh```. This script makes use of ```colonel``` behind the scenes. The only difference here is that we use the application's name instead of its PID.
You can also use `
This script finds the pid for the application you want to change and will invoke ```colonel```.
```
sh ros2_topic_changer.sh <action> <application_name> <old_topic_name> <new_topic_name>
```
    
        
### Example Policy

We now demonstrate the working of a policy referred to as 'blur-exported-images', which ensures that all images published from a ROS2 camera application are redirected through a trusted filter, which blurs them before they are used. This is an example of a policy that hosts of restricted airspaces may which to use, so that any images that leave the drone (possibly via the network) are blurred, thereby preserving privacy.

The initial setup consists of two ROS2 applications:
* ```camera``` - reads images from a source and publishes them via topic 'imageraw'
* ```showimage``` - receives images from 'camera' by subscribing to topic 'imageraw'

Launch these two applications by executing the following commands:

```
ros2 run image_tools camera
ros2 run image_tools showimage
```

To enforce the 'blur-exported-images' policy, we must redirect images from ```camera``` to a trusted filter application known as ```blurfilter```, such that ```showimage``` only receives blurred images. To achieve this, we dynamically redirect published messages from ```camera``` to ```blurfilter``` by using a new topic 'tmp1'. Next, ```blurfilter``` publishes blurred images to the original topic 'imageraw'. As a result, ```showimage``` receives blurred images without requiring any modification to its source code or run-time behaviour. Thus, Privaros has seamlessly inserted a trusted filter application to alter the information flow across ROS2 applications. 


Here are the steps to enforce 'blur-exported-images':
```
# Launch the trusted filter by linking it against the 'blurring' library
ros2 run image_tools blurfilter


# Redirect information flow to blur images:
# camera publishes to 'tmp1'
# blurfilter subscribes to 'tmp1'
# blurfilter publishes to 'imageraw'
ros2_topic_changer.sh sub blurfilter input tmp1 pub blurfilter output imageraw pub camera imageraw tmp1 
```

> Our changes to ROS2 ensure that only trusted applications are allowed to publish to other topics (in addition to the over-provisioned topics discussed earlier) such as 'imageraw' in the above example.

The source files for all 3 applications can be found at ```$ROS2_WS/src/ros2/demos/templates/src```
    
If you encounter problems, here are a few suggestions that may help resolve them:



* ```blurfilter``` and ```colonel``` are trusted applications, and must therefore have the following rule as part of their AppArmor profiles:
    ```
    is_trusted true
    ```
    
* Both the ```camera``` and ```showimage``` applications must have the following rules as part of their AppArmor profiles:
    ```
    dte_net_domain {camera_hash},
    dte_net_domain {showimage_hash},
    dte_net_domain {blurfilter_hash},
    dte_net_domain {colonel_hash},
    ```
    This basically means that Privaros' LSM will allow ```camera``` and ```showimage``` to communicate with all the applications listed above.

* For any other errors, kindly refer to the [Running ROS Applications](#Running-ROS-Applications) section above.

##  Contact
* Rakesh Rajan Beck <rakeshbeck@iisc.ac.in>
* Abhishek Vijeev <abhishekvijeev@iisc.ac.in>

