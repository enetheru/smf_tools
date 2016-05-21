#!/bin/bash
# this script is intended to wrap the cli smf tools to make creating maps easier.
# the idea is that given the most simple situation, creating a map should be easy

# Set needed variables in case of bash shell conflicts
##bc#e###ij########s#u##x##ABC#EFGHIJKLM#OPQRSTUVWX#Z
#TODO add version with -V
#TODO add option to start game after compilation complete wit -G
#TODO add option to link compiled game directory into spring map folder with -L
#TODO add option to compress into archive with -Z maybe make -L switch to
#     moving archive to folder
#TODO e can specify environment settings like atmospphere and lighting?

HELPSHORT="makemap - smf_tools helper script that generates a working map
usage: makemap [options]
  eg.   makemap -vpf -n MyMap -d diffuse.jpg -h height.tif -o ./"
HELP=$HELPSHORT"

  -h            this help message
  -v            verbose output
  -p            progress bar
  -q            dont ask questions
  -o <path>     output directory name
  -k            overwrite files (k for keep)

  -n <string>   name of the map
  -N <string>   long name
  -D <string>   description
  -w <integer>  map width in spring map units
  -l <integer>  map length in spring map units
  -y <float>    map depth
  -Y <float>    map height

  -m <image>    minimap image
  -d <image>    diffuse texture image
  -a <image>    height image (a for altitude)

  -g <image>    grass distribution image
  -f <file>     features file

  -t <image>    type map image
  -r <image>    metal distribution image (r for resources)

  -z <string>   map water level (z for ground zero)
"
# Options
# =======
# form of option goes is
# OPTION=( "value" "default" "regex" "help" "question" )
# where:
#   default = the default value, can be ""
#   regex = the appropriate evaluator for valid input
#   help = maximum of three line help indicating what the option is about
#   question = in menu mode what to ask the user to be polite

REGEX_FLOAT="^[+-]?[0-9]*\.?[0-9]+$"

VERBOSE=( "n" "^[yn]$" \
"v:verbose - specifying this options increases the output presented to the
command line" \
"would you like to get more detailed output?" )
PROGRESS=( "n" "^[yn]$" \
"p:progress - progress bars will be displayed for long operations when possible" \
"would you like to see progress bars?" )
QUIET=( "n" "^[yn]$" \
"q:quiet - supressed the output including error messages." \
"would you like to silence the output?" )
OUTPUT_PATH=( "." "^.*$" \
"o:output path - the directory in which to place output files." \
"please enter the path where you would like files to be saved." )
OUTPUT_OVERWRITE=( "n"  "^[yn]$" \
"f:overwrite files - whether or not pre-existing files should be overwritten" \
"would you like to clobber pre-existing files?" )

MAP_NAME=( "" '^[a-zA-Z0-9_]+$' \
"n:name - the short name of the map used to save files" \
"please name the map." )
MAP_LONGNAME=( "" '^.+$' \
"N:display name - The name used in game" \
"please provide a display name" )
MAP_DESCRIPTION=( "" '^.+$' \
"D:description - a short description of the map" \
"Please describe the map" )
MAP_BREADTH=( "" "^[0-9]+$" \
"w:map width - defined in spring map units, 1:512 pixel ratio" \
"mapwidth question?" )
MAP_LENGTH=( "" "^[0-9]+$" \
"help" \
"map length" )
MAP_FLOOR=( "" "$REGEX_FLOAT" \
"help" \
"map floor" )
MAP_CEILING=( "" "$REGEX_FLOAT" \
"help" \
"map ceiling" )

MAP_DIFFUSEIMAGE=( "" "^.*$" \
"help" \
"diffuse image" )
MAP_MINIIMAGE=( "" "^.*$" \
"help" \
"mini image" )
MAP_HEIGHTIMAGE=( "" "^.*$" \
"help" \
"heigtimage" )

MAP_GRASSIMAGE=( "" "^.*$" \
"help" \
"grassimage" )
MAP_FEATURES=( "" "^.*$" \
"help" \
"features" )

MAP_TYPEIMAGE=( "" "^.*$" \
"help" \
"typeimage" )
MAP_METALIMAGE=( "" "^.*$" \
"help" \
"metalimage" )

MAP_WATERLEVEL=( "" "^.*$" \
"help" \
"waterlevel" )


#TODO generate manpage
#TODO how to set the water level, by pixel or by percent etc
#     one option is that setting the y-Y values is used for total size, and the
#     z value either in pixel, or percent pushes the yY values to the appropriate position
#TODO howto specify the types of terrain and how they relate to the finished archive
#TODO howto specify features and how they related to the finished archive
#TODO option to smooth samples in a heightmap, one method that might work is
#     ito double the resolution of the image interpolating the values, then
#     scale back down preserving the sharp features.
#TODO guess approriate height values based in input height image.
#TODO how do I go about specifying lighting options on the command line, let
#     along half the other shit inside a spring map.

# NOTE using getopts, an alternative might be to use docopts, but i am loath to
#      add a dependency
# NOTE the bash builtin getopts is different than getopt which aparrantly can
#      do long opts. might be worth having a look

if ( ! getopts "hvpqkonwlyYmdazgftr" opt)
then
    echo "$HELPSHORT"
fi
while getopts "hvpqko:n:N:D:w:l:y:Y:m:d:a:z:g:f:t:r:" opt; do
    case $opt in
        h) echo "$HELP"; exit;;
        v) VERBOSE='y';;
        p) PROGRESS='y';;
        q) QUIET='y';;
        k) OUTPUT_OVERWRITE='y';;
        o) OUTPUT_PATH=$OPTARG;;
        n) MAP_NAME=$OPTARG;;
        N) MAP_LONGNAME=$OPTARG;;
        D) MAP_DESCRIPTION=$OPTARG;;
        w) MAP_BREADTH=$OPTARG;;
        l) MAP_LENGTH=$OPTARG;;
        y) MAP_FLOOR=$OPTARG;;
        Y) MAP_CEILING=$OPTARG;;
        m) MAP_MINIIMAGE=$OPTARG;;
        d) MAP_DIFFUSEIMAGE=$OPTARG;;
        a) MAP_HEIGHTIMAGE=$OPTARG;;
        g) MAP_GRASSIMAGE=$OPTARG;;
        f) MAP_FEATURES=$OPTARG;;
        t) MAP_TYPEIMAGE=$OPTARG;;
        r) MAP_METALIMAGE=$OPTARG;;
        z) MAP_WATERLEVEL=$OPTARG;;
    esac
done

OptionAsk()
{
    # messed up shit to get array indirect referencing working
    temp=$1'[@]'
    OPTION=( "${!temp}" )

    echo -e "${OPTION[2]}"
    echo -e "${OPTION[3]}"
    read -re -p "${OPTION[1]}: " $1

    #regex match testing
    if [[ ! "${!1}" =~ ${OPTION[1]} ]]
    then
        echo you entered: \"${!1}\" unfortunately that does not match the regex \
            pattern listed, please try again.
        OptionAsk $1
    fi
#TODO Validate input, this could be nested function calls with the validation
# on a different layer ie OptionAskImage
}


OptionsReview()
{
    echo \
"Confirm Options:
================

General options:
  v:verbose               : $VERBOSE
  p:progress              : $PROGRESS
  q:quiet                 : $QUIET
  o:output path           : $OUTPUT_PATH
  k:overwrite             : $OUTPUT_OVERWRITE

Map Properties:
  n:name          required: $MAP_NAME
  N:pretty name           : $MAP_LONGNAME
  D:description           : $MAP_DESCRIPTION
  w:width         required: $MAP_BREADTH
  l:length        required: $MAP_LENGTH
  y:depth                 : $MAP_FLOOR
  Y:height                : $MAP_CEILING

  m:mini image            : $MAP_MINIIMAGE
  d:diffuse image required: $MAP_DIFFUSEIMAGE
  a:height image          : $MAP_HEIGHTIMAGE

  g:grass image           : $MAP_GRASSIMAGE
  f:features file         : $MAP_FEATURES

  t:type image            : $MAP_TYPEIMAGE
  r:metal image           : $MAP_METALIMAGE

Pre-Processing Functions
  z:water level

  Q: Nooooo, get me out of here!!!
choose option, or press enter to confirm:"
    read RETVAL
    if [[ ! $RETVAL ]]
    then
        RETVAL=C
    fi
}

ValidateOptions()
{
    #test each option for validity and provide an error message if it aint so.
    if [[ ! $MAP_LONGNAME ]]
    then
        MAP_LONGNAME=$MAP_NAME
    fi

    if [[ ! $MAP_BREADTH ]]; then OptionAsk MAP_BREADTH; fi
    if [[ ! $MAP_LENGTH ]]; then OptionAsk MAP_LENGTH; fi
    if [[ ! $MAP_FLOOR ]]; then OptionAsk MAP_FLOOR; fi
    if [[ ! $MAP_CEILING ]]; then OptionAsk MAP_CEILING; fi

}

# Review Options
# ==============
UNHAPPY=1
while [[ $UNHAPPY -eq 1 ]]
do
    clear
    OptionsReview
    case $RETVAL in
        v) OptionAsk VERBOSE;;
        p) OptionAsk PROGRESS;;
        q) OptionAsk QUIET;;
        k) OptionAsk OUTPUT_OVERWRITE;;
        o) OptionAsk OUTPUT_PATH;;
        n) OptionAsk MAP_NAME;;
        N) OptionAsk MAP_LONGNAME;;
        D) OptionAsk MAP_DESCRIPTION;;
        w) OptionAsk MAP_BREADTH;;
        l) OptionAsk MAP_LENGTH;;
        y) OptionAsk MAP_FLOOR;;
        Y) OptionAsk MAP_CEILING;;
        m) OptionAsk MAP_MINIIMAGE;;
        d) OptionAsk MAP_DIFFUSEIMAGE;;
        a) OptionAsk MAP_HEIGHTIMAGE;;
        g) OptionAsk MAP_GRASSIMAGE;;
        f) OptionAsk MAP_FEATURES;;
        t) OptionAsk MAP_TYPEIMAGE;;
        r) OptionAsk MAP_METALIMAGE;;
        z) OptionAsk MAP_WATERLEVEL;;
        Q) exit;;
        C) UNHAPPY=0;;
    esac

    ValidateOptions
done

# create folder structure
echo Creating Directory Structure
BASE_PATH=${OUTPUT_PATH}/${MAP_NAME}.sdd
mkdir -p $BASE_PATH/{maps,LuaGaia/Gadgets}

# populate Feature Placer files
# -----------------------------
echo -e '\n[makemap]Creating boilerplate files for featureplacer'

# LuaGaia/main.lua
#TODO dont clobber if already existsa
FILENAME=$BASE_PATH/LuaGaia/main.lua
FILECONTENT='if AllowUnsafeChanges then AllowUnsafeChanges("USE AT YOUR OWN PERIL") end
VFS.Include("LuaGadgets/gadgets.lua",nil, VFS.BASE)'
if [[ -e $FILENAME && $OUTPUT_OVERWRITE == 'y' ]]; then echo $FILECONTENT > $FILENAME; fi

# LuaGaia/draw.lua
#TODO dont clobber if already exists
echo 'VFS.Include("LuaGadgets/gadgets.lua",nil, VFS.BASE)' > $BASE_PATH/LuaGaia/draw.lua

# LuaGaia/Gadgets/featureplacer.lua
#TODO dont clobber if already exists
echo 'function gadget:GetInfo()
    return {
        name      = "feature placer",
        desc      = "Spawns Features and Units",
        author    = "Gnome, Smoth",
        date      = "August 2008",
        license   = "PD",
        layer     = 0,
        enabled   = true  --  loaded by default?
    }
end

if (not gadgetHandler:IsSyncedCode()) then
  return false
end

if (Spring.GetGameFrame() >= 1) then
  return false
end

local SetUnitNeutral        = Spring.SetUnitNeutral
local SetUnitBlocking       = Spring.SetUnitBlocking
local SetUnitRotation       = Spring.SetUnitRotation
local SetUnitAlwaysVisible  = Spring.SetUnitAlwaysVisible
local CreateUnit            = Spring.CreateUnit
local CreateFeature         = Spring.CreateFeature

local features = VFS.Include("features.lua")
local objects = features.objects
local buildings = features.buildings
local units     = features.units


local gaiaID = Spring.GetGaiaTeamID()

for i,fDef in pairs(objects) do
    local flagID = CreateFeature(fDef.name, fDef.x, Spring.GetGroundHeight(fDef.x,fDef.z)+5, fDef.z, fDef.rot)
end

local los_status = {los=true, prevLos=true, contRadar=true, radar=true}

for i,uDef in pairs(units) do
    local flagID = CreateUnit(uDef.name, uDef.x, 0, uDef.z, 0, gaiaID)
    SetUnitRotation(flagID, 0, -uDef.rot * math.pi / 32768, 0)
    SetUnitNeutral(flagID,true)
    Spring.SetUnitLosState(flagID,0,los_status)
    SetUnitAlwaysVisible(flagID,true)
    SetUnitBlocking(flagID,true)
end

for i,bDef in pairs(buildings) do
    local flagID = CreateUnit(bDef.name, bDef.x, 0, bDef.z, bDef.rot, gaiaID)
    SetUnitNeutral(flagID,true)
    Spring.SetUnitLosState(flagID,0,los_status)
    SetUnitAlwaysVisible(flagID,true)
    SetUnitBlocking(flagID,true)
end

return false --unload' > $BASE_PATH/LuaGaia/Gadgets/featureplacer.lua

# TODO generate this file from source material rather than an empty one
# features.lua
#TODO dont clobber if already exists
echo 'local lists = {
    units = {
        --{ name = 'template', x = 512, z = 512, rot = "0" },
    },
    buildings = {
        --{ name = 'template', x = 512, z = 512, rot = "0" },
    },
    objects = {
        --{ name = 'template', x = 512, z = 512, rot = "0" },
    },
}
return lists' > $BASE_PATH/features.lua

# Map Info
# ========
# mapinfo.lua
#TODO dont clobber if already exists
echo "--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- mapinfo.lua
--

local mapinfo = {
    name        = '$MAP_LONGNAME',
    shortname   = '$MAP_NAME',
    description = '$MAP_DESCRIPTION',
    author      = '$USER',
    version     = '$VERSION',
    modtype     = 3, --// 1=primary, 0=hidden, 3=map
    depend      = {'Map Helper v1'},
    replace     = {},

    maphardness     = 100,
    notDeformable   = false,
    gravity         = 130,
    tidalStrength   = 0,
    maxMetal        = 0.02,
    extractorRadius = 500.0,
    voidWater       = true,
    autoShowMetal   = true,


    smf = {
        minheight = $MAP_FLOOR,
        maxheight = $MAP_CEILING,
    },

    splats = {
        texScales = {0.02, 0.02, 0.02, 0.02},
        texMults  = {1.0, 1.0, 1.0, 1.0},
    },

    atmosphere = {
        minWind      = 5.0,
        maxWind      = 25.0,

        fogStart     = 0.1,
        fogEnd       = 1.0,
        fogColor     = {0.7, 0.7, 0.8},

        sunColor     = {1.0, 1.0, 1.0},
        skyColor     = {0.1, 0.15, 0.7},
        skyDir       = {0.0, 0.0, -1.0},

        cloudDensity = 0.5,
        cloudColor   = {1.0, 1.0, 1.0},
    },

    grass = {
        bladeWaveScale = 1.0,
        bladeWidth  = 0.32,
        bladeHeight = 4.0,
        bladeAngle  = 1.57,
        bladeColor  = {0.59, 0.81, 0.57}, --// does nothing when 'grassBladeTex' is set
    },

    lighting = {
        --// dynsun
        sunStartAngle = 0.0,
        sunOrbitTime  = 1440.0,
        sunDir        = {0.0, 1.0, 2.0, 1e9},

        --// unit & ground lighting
        groundAmbientColor  = {0.5, 0.5, 0.5},
        groundDiffuseColor  = {0.5, 0.5, 0.5},
        groundSpecularColor = {0.1, 0.1, 0.1},
        groundShadowDensity = 0.8,
        unitAmbientColor    = {0.4, 0.4, 0.4},
        unitDiffuseColor    = {0.7, 0.7, 0.7},
        unitSpecularColor   = {0.7, 0.7, 0.7},
        unitShadowDensity   = 0.8,

        specularExponent    = 100.0,
    },

    water = {
        damage =  0.0,

        repeatX = 0.0,
        repeatY = 0.0,

        absorb    = {0.0, 0.0, 0.0},
        baseColor = {0.0, 0.0, 0.0},
        minColor  = {0.0, 0.0, 0.0},

        ambientFactor  = 1.0,
        diffuseFactor  = 1.0,
        specularFactor = 1.0,
        specularPower  = 20.0,

        planeColor = {0.0, 0.4, 0.0},

        surfaceColor  = {0.75, 0.8, 0.85},
        surfaceAlpha  = 0.55,
        diffuseColor  = {1.0, 1.0, 1.0},
        specularColor = {0.5, 0.5, 0.5},

        fresnelMin   = 0.2,
        fresnelMax   = 0.8,
        fresnelPower = 4.0,

        reflectionDistortion = 1.0,

        blurBase      = 2.0,
        blurExponent = 1.5,

        perlinStartFreq  =  8.0,
        perlinLacunarity = 3.0,
        perlinAmplitude  =  0.9,
        windSpeed = 1.0, --// does nothing yet

        shoreWaves = true,
        forceRendering = false,
    },

    teams = {
        [0] = {startPos = {x = 128, z = 128}},
        [1] = {startPos = {x = 1024-128, z = 1024-128}},
    },

    terrainTypes = {
        [0] = {
            name = 'Default',
            hardness = 1.0,
            receiveTracks = true,
            moveSpeeds = {
                tank  = 1.0,
                kbot  = 1.0,
                hover = 1.0,
                ship  = 1.0,
            },
        },
    },
}

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Helper

local function lowerkeys(ta)
    local fix = {}
    for i,v in pairs(ta) do
        if (type(i) == 'string') then
            if (i ~= i:lower()) then
                fix[#fix+1] = i
            end
        end
        if (type(v) == 'table') then
            lowerkeys(v)
        end
    end

    for i=1,#fix do
        local idx = fix[i]
        ta[idx:lower()] = ta[idx]
        ta[idx] = nil
    end
end

lowerkeys(mapinfo)

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

return mapinfo" > $BASE_PATH/mapinfo.lua

# Run the smt_convert command to create tile file from images
# -----------------------------------------------------------
echo -e '\n[makemap]Generating the smt file'
COMMAND='smt_convert'
if [[ $VERBOSE == 'y' ]]; then COMMAND+=' -v'; fi
if [[ $PROGRESS == 'y' ]]; then COMMAND+=' -p'; fi
if [[ $QUIET == 'y' ]]; then COMMAND+=' -q'; fi
if [[ $OUTPUT_OVERWRITE == 'y' ]]; then COMMAND+=' -f'; fi
COMMAND+=" --imagesize $((512 * $MAP_BREADTH))x$((512 * $MAP_LENGTH))"
COMMAND+=' --smt --tilesize 32x32 --type DXT1'
COMMAND+=" -o $BASE_PATH/maps/${MAP_NAME}.smt"
COMMAND+=" $MAP_DIFFUSEIMAGE"
echo $COMMAND
$COMMAND

# run the smf_cc command to create the smf file
# ---------------------------------------------
echo -e '\n[makemap]Generating the smf file'
COMMAND='smf_cc'
if [[ $VERBOSE == 'y' ]]; then COMMAND+=' -v'; fi
#if [[ $PROGRESS == 'y' ]]; then COMMAND+=' -p'; fi # maybe add a progress indicator to smf_cc?
if [[ $QUIET == 'y' ]]; then COMMAND+=' -q'; fi
if [[ $OUTPUT_OVERWRITE -eq 'y' ]]; then COMMAND+=' -f'; fi
COMMAND+=" --mapsize ${MAP_BREADTH}x${MAP_LENGTH}"
COMMAND+=' --tilesize 32'
COMMAND+=" -y $MAP_FLOOR"
COMMAND+=" -Y $MAP_CEILING"
COMMAND+=" --tilemap $BASE_PATH/maps/$MAP_NAME.smt.csv"
if [[ $MAP_HEIGHTIMAGE ]]; then COMMAND+=" --height $MAP_HEIGHTIMAGE"; fi
if [[ $MAP_TYPEIMAGE ]]; then COMMAND+=" --type $MAP_TYPEIMAGE"; fi
if [[ $MAP_MINIIMAGE ]]; then COMMAND+=" --mini $MAP_MINIIMAGE"; fi
if [[ $MAP_METALIMAGE ]]; then COMMAND+=" --metal $MAP_METALIMAGE"; fi
if [[ $MAP_GRASSIMAGE ]]; then COMMAND+=" --grass $MAP_GRASSIMAGE"; fi
COMMAND+=" -o $BASE_PATH/maps/${MAP_NAME}.smf"
COMMAND+=" $BASE_PATH/maps/$MAP_NAME.smt"
echo "[makemap]$COMMAND"
$COMMAND

# delete superflous tilefile
rm $BASE_PATH/maps/${MAP_NAME}.smt.csv

# Link the generated game folder into the spring settings folder
# --------------------------------------------------------------
LINKFOLDER=y
if [[ $LINKFOLDER == 'y' ]]
then
    echo -e '\n[makemap]Linking generated map archive into spring config folders'
    COMMAND='ln -s'
    if [[ $OUTPUT_OVERWRITE == 'y' ]]; then COMMAND+='f'; fi
    #TODO get spring maps folder from spring configuration file
    COMMAND+=" `pwd`/${MAP_NAME}.sdd /home/$USER/.config/spring/maps"
    echo "[makemap]$COMMAND"
    $COMMAND
fi

# Run the Game to test the map
# ----------------------------
if [[ $GAMEON == 'y' ]]; then spring -m $MAP_NAME -g 'Empty Mod'; fi

# Remaining Items on my thought list
# ----------------------------------
#TODO change double qotes " to single quotes ' if variable substitution is not nexessary
#TODO delete beans.smt.csv
#TODO validate input mapsize to be multiples of two, and update the help text to specify it.
#TODO validate input images
#TODO create a link to the users spring config/maps directory for testing
#TODO option to run the game using the map immediately after creation to test that it works as expected.

#TODO for lighting give presets like sunset, sunrise, midday, dusk, night etc..
#TODO automatically detect sizes of things based on image dimensions
#TODO display the completed makemap command to the user now that any mistakes have been corrected
#TODO run appropriate smf_cc and smt_convert functions and place the outputs into the approriat places

#Future maybe
# optionally create mapinfo/* files depending on options. but i think i might remove them in this version
# give presets for weather effects, like light rain, heavy rain, snow, windy etc.
# TODO use some form of toolkit to allow file selection using GUI
# NOTE yad is a good option to use for dialog based formsw
