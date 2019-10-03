A scalable pipeline for designing reconfigurable organisms.
--------------------
[Sam Kriegman](https://scholar.google.com/citations?user=DCIwaLwAAAAJ), 
[Douglas Blackiston](https://scholar.google.com/citations?user=d2Jd2jsAAAAJ), 
[Michael Levin](https://scholar.google.com/citations?user=luouyakAAAAJ), 
[Josh Bongard](https://scholar.google.com/citations?user=Dj-kPasAAAAJ).<br>


Installation
------------

The first step is to install [Anaconda](http://docs.continuum.io/anaconda/install/linux/) as your 
<b>python 2.7</b> distribution on a linux machine.

But revert Anaconda's networkx <2.0.

    pip install networkx==1.11

Install Qt and QMake, specifically these packages: "libqt4-dev", "qt4-qmake", "libqwt-dev", "freeglut3-dev" and "zlib1g-dev".

    sudo apt-get install libqt4-dev qt4-qmake libqwt-dev freeglut3-dev zlib1g-dev


Install git.

    sudo apt-get install git

Navigate to the working directory (e.g., your home).

    cd ~

Clone the repo.

    git clone https://github.com/skriegman/reconfigurable_organisms.git

Navigate to the _voxcad directory.

    cd reconfigurable_organisms/_voxcad/

Compile both VoxCad (GUI) and Voxelyze (physics engine).

    ./rebuild_everything.sh

If you happen to modify VoxCad or Voxelyze in the future, call the same script to clean and recompile everything. 

    make

Install the voxelyze library.

    cd Voxelyze
    make
    make installusr
    cd ../voxelyzeMain/
    make

Navigate to the exp folder and run one of the .py experiments (detailed in the paper),
which require two input args (seed and runtime), both of which can be set to 1 for now.
    
    cd ../exp
    PYTHONPATH=$HOME/reconfigurable_organisms/ $HOME/anaconda/bin/python Locomotion_pass2.py 1 1

This creates a directory (~/reconfigurable_organisms/run_1) to hold the results.

Output should appear in the console.

After the experiment runs for a few generations, 
the current shape/controller adaptation can be seen by opening 
one of the generated .vxa files within the VoxCAD GUI. 
A .vxa file is just an XML file representing a robot that can be simulated by VoxCad/Voxelyze.

Navigate to reconfigurable_organisms/_voxcad/release.
    
    cd ../_voxcad/release
    
Open VoxCad.

    ./VoxCad

Then select the desired .vxa file from:

    "File -> Import -> Simulation"

The .vxa files for the best performing individuals will be saved in:

    reconfigurable_organisms/exp/run_1/bestSoFar/fitOnly.

Once the robot is loaded, you can start the physics simulation by clicking the <img src="https://github.com/skriegman/reconfigurable_organisms/blob/master/_voxcad/VoxCad/Icons/Sandbox.png" height="25" width="25"> icon in the top bar ("Physics Sandbox").


A known issue with non-US machines
--------

If the system's numeric 
<a href="https://en.wikipedia.org/wiki/Locale_(computer_software)">locale</a> 
differs from en_US.UTF-8, the 
<a href="http://www.cplusplus.com/reference/cstdlib/atof/">atof</a> 
function might read double and floating point values in the .vxa 
as integers.
 
This will cause the robot not to move, disappear, or just behave strangely 
when running a .vxa file in VoxCad.

To fix this issue, open the locale settings.

    sudo gedit /etc/default/locale

Set LC_NUMERIC to en_US.UTF-8.

    LC_NUMERIC="en_US.UTF-8"

Save, close the file, and reboot.


Documentation
-------------

Voxelyze documentation is available [here](http://jonhiller.github.io/Voxelyze/annotated.html).


