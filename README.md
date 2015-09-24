## Downloading source
Install git if you do not have it. Download the source from gitlab:

    $ git clone git@gitlab.cs.washington.edu:syslab/diamond-src.git

## Building Diamond for x86

You can compile Diamond on Mac OSX and Linux for x86 using the
instructions below. Diamond has language binding for Java, Android,
Objective-C, Python and C++. In order to build the bindings in each
case, you'll need to set up different things.

### Set up the build environment

Diamond requires a few common tools to compile for various languages,
which you can download using your package manager of choice (e.g. apt
on Ubuntu/Debian or Macports/Homebrew on OSX):

* C++ bindings: Install CMake and either gcc or clang. Diamond builds
  as a C++ shared library by default.
* Python bindings: Install the Python Boost modules
* Java bindings: Install Maven and the JDK for compiling the Java bindings

For Ubuntu 14.04, installing the following packages should be 
sufficient to build Diamond:
   $ apt-get install build-essential cmake maven libpython-dev libboost-dev libuv-dev

### Building the source and language bindings
To compile a shared object library and both the C++ and Python
bindings:

	$ cd backend
	$ mkdir build
	$ cd build
	$ cmake ..
	$ make

To run tests for the C++ and Python bindings type in the `build` directory:

    $ make diamondtest

After you have compiled the C++ shared library, you can compile the
Java bindings for those libraries: 

	$ cd backend/src/bindings/java
    $ mvn package

To test the Java bindings, cd to the Java directory and type:

    $ mvn test


## Cross-compiling Diamond for Android

In order to cross-compile for using Diamond on Android or iOS, you
need to download the compile tools. Create a `toolchains` directory in
the `backend` directory of the Diamond source code and keep everything
that you downloaded there.

### Android
1. Install either Android Studio or the stand-alone SDK.
2. Create a directory for the Android NDK tools:

        $ mkdir backend/toolchains/android

2. Download the Android NDK from [here](http://developer.android.com/ndk/downloads/index.html).

3. The NDK unpacks itself so run:

        $ chmod u+x android-ndk-r10e-linux-x86_64.bin
        $ ./android-ndk-r10e-linux-x86_64.bin
		$ mv android-ndk-r10e-linux-x86_64 backend/toolchains/android/ndk

    Be sure to use the appropriate '.bin' filename for OSX.

4. Create a stand-alone toolchain for working with. On Linux:

		$ cd backend/toolchains/android
		$ ./ndk/build/tools/make-standalone-toolchain.sh --toolchain=arm-linux-androideabi-4.9 --arch=arm --platform=android-21 --install-dir=toolchain

5. Compile the C++ Diamond library:
		
		$ cd backend
		$ mkdir build-arm
		$ cd build-arm
		$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../Android.cmake
		$ make

6. Compile Java bindings for Diamond:

		$ cd backend/src/bindings/java
		$ mvn package

### Running a test Android app
The project DiamondAndroidTest in the "apps/test-apps" folder is an Eclipse project containing a simple Diamond test app for Android.
The following instructions describe how to set up and run the app. Replace $DIAMOND_SRC with the path to the base Diamond
source directory and $ANDROID_SDK with the path to the Android SDK folder.

1. Run the script "build-diamond-android.sh" to build the Java bindings and copy all required shared libraries into the project folder:

        $ cd scripts
        $ ./build-diamond-android.sh $DIAMOND_SRC $DIAMOND_SRC/apps/test-apps/DiamondAndroidTest

2. In Eclipse, import the DiamondAndroidTest project:

    1. Go to File -> Import -> Existing Projects into Workspace.
    2. Select $DIAMOND_SRC/apps/test-apps/DiamondAndroidTest as the root directory.

3. Add the Android support v7 appcompat library to the project:

    1. Go to File -> Import -> Existing Projects into Workspace.
    2. Select $ANDROID_SDK/extras/android/support/v7/appcompat as the root directory.
    3. Go to Project -> Properties -> Android. Under Library, select Add, then select appcompat_v7.

4. Go to Window -> Preferences -> Android -> Build, and uncheck the option "Force error when external jars contain native libraries."

5. Refresh, clean, and close and open the project until it builds successfully:

    1. To refresh: right-click on the project in the Package Explorer -> Refresh
    2. To clean: select Project -> Clean
    3. To close and open: right-click on the project -> Close Project (or Open Project)

6. Run the project as an Android application:

    1. Right click on the project name in the Package Explorer pane.
    2. Select "Run As" -> "Android Application."
    3. Choose a device (either an emulator or a physical device) to run on.