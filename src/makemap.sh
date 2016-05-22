#!/bin/bash
# this script is intended to wrap the cli smf tools to make creating maps easier.
# the idea is that given the most simple situation, creating a map should be easy
# return values
# 0 = successful
# 1 = input commands invalid
# 2 = failure to do something

# Set needed variables in case of bash shell conflicts
#TODO add version with -V
#TODO add option to compress into archive with -Z maybe make -L switch to
#     moving archive to folder
#TODO e can specify environment settings like atmospphere and lighting?
#TODO check all recent development and update script to be as modern as possible
# * lua overrides for maps

# NOTE: have to think about things that can go wrong, work out a strategy to
# work on identifying and fixing the technical debt accumulated over the 2 days
# i have been hacking this together, separating the Ideas from the TODO items
# Line by line

##bc#e####jk#######s#u##x##ABC#E#GHIJKLM#OPQRSTUVWX##
HELPSHORT="makemap - smf_tools helper script that generates a working map
usage: makemap [options]
  eg.   makemap -vpf -n MyMap -d diffuse.jpg -h height.tif -o ./"
HELP=$HELPSHORT"

Script Operation:
  -h            this help message
  -q            supress output including errors
  -v            verbose output
  -p            progress bara
  -i            interactive mode
  -f            overwrite files (k for keep)
  -o <path>     output directory name
  -Z            7zip the archive
  -L            link map directory or archive into spring settings/maps folder
  -G            test game afterwards, assumes -L

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
  -F <file>     features file

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

REGEX_FLOAT='^[+-]?[0-9]*\.?[0-9]+$'
REGEX_YN='^[yn]?$'
REGEX_PATH='^.+$'

VERBOSE=( 'n' "$REGEX_YN" \
'v:verbose - specifying this options increases the output presented to the
command line' \
'would you like to get more detailed output?' )
PROGRESS=( 'n' "$REGEX_YN" \
'p:progress - progress bars will be displayed for long operations when possible' \
'would you like to see progress bars?' )
INTERACTIVE=( 'n' "$REGEX_YN" \
'I:interactive - This mode allows you to correct any mistakes, or select
options using a menu' '' )
GAMEON=( 'n' "$REGEX_YN" \
'G:test with game - Test the newly constructed map in the engine
this will assume -L.'
'would you like to test the map in the engine?' )
QUIET=( 'n' "$REGEX_YN" \
"q:quiet - supressed the output including error messages." \
"would you like to silence the output?" )
OUTPUT_PATH=( '.' "$REGEX_PATH" \
'o:output path - the directory in which to place output files.' \
'please enter the path where you would like files to be saved.' )
OUTPUT_OVERWRITE=( 'n'  "$REGEX_YN" \
'f:overwrite files - whether or not pre-existing files should be overwritten' \
'would you like to clobber pre-existing files?' )
OUTPUT_COMPRESS=( 'n' "$REGEX_YN" \
'Z:7zip map - will 7zip the archive in the correct format for distribution'
'would you like to 7zip the map?' )
OUTPUT_LINK=( 'n' "$REGEX_YN" \
'L:link archive - link the newly constructed map archive to the spring maps folder.' \
'would you like to link the map archive/folder to the spring maps folder?' )

MAP_NAME=( '' '^[a-zA-Z0-9_]+$' \
'n:name - the short name of the map used to save files' \
'please enter a name for the map.' )
MAP_LONGNAME=( '' '^.+$' \
'N:display name - ' \ #FIME, find out what the display name is used for
'please provide a display name.' )
MAP_DESCRIPTION=( '' '^.+$' \ #FIME, find out what the display name is used for
'D:description - a short description of the map' \
'please describe the map.' )
MAP_BREADTH=( '' '^[0-9]+$' \
'w:map breadth - defined in spring map units, 1:512 spring:pixel ratio' \
'Please enter the breadth.' )
MAP_LENGTH=( '' '^[0-9]+$' \
'l:map length - defined in spring map units, 1:512 spring:pixel ratio" \' \
'please enter the length' )
MAP_FLOOR=( '' "$REGEX_FLOAT" \
'y:map floor - the lowest point in the map, defines the value of black pixels
in the height image, 0 is water, negative numbers are underwater, positive
numbers are above water. also have a 1:1 ratio with pixels from the diffuse.' \
'please enter the floor.' )
MAP_CEILING=( '' "$REGEX_FLOAT" \
'Y:map ceiling - the highest point in the map, defines the value of white pixels
in the height image, 0 is water, negative numbers are underwater, positive
numbers are above water. also have a 1:1 ratio with pixels from the diffuse.' \
'please enter the ceiling' )

MAP_DIFFUSEIMAGE=( '' "$REGEX_PATH" \
'd:diffuse image - defines the primary colour of the ground surface, has a
1:512 ratio spring map unit:pixels, anything not meeting this will be scaled' \
'please enter the path of the diffuse image, autocomplete via readline is enabled' )
MAP_MINIIMAGE=( '' "$REGEX_PATH" \
'm:minimap image - a 1024x1024 rgba image used to display a minimap in game.
images not conforming to this spec will be scaled.' \
'please enter the path of the minimap, autocommplete via readline is enabled' )
MAP_HEIGHTIMAGE=( '' "$REGEX_PATH" \
'h:height image - a ratio of 1:8 rgba image used to display a minimap in game.
images not conforming to this spec will be scaled.' \
'please enter the path of the height, autocommplete via readline is enabled' )

MAP_GRASSIMAGE=( '' "$REGEX_PATH" \
'help' \
'please enter the path of the grass image, autocommplete via readline is enabled' )
MAP_FEATURES=( '' "$REGEX_PATH" \
'help' \
'please enter the path of the features definition, autocomplete via readline enabled' )

MAP_TYPEIMAGE=( '' "$REGEX_PATH" \
'help' \
'please enter the path of the type map, autocommplete via readline is enabled' )
MAP_METALIMAGE=( '' "$REGEX_PATH" \
'help' \
'please enter the path of the metal map, autocommplete via readline is enabled' )

MAP_WATERLEVEL=( '' "$REGEX_PATH" \
'help' \
'waterlevel' )

# FIXME peform guesswork here, ie use image width/height to populate size values
# TODO use heightmap values to populate yY values

#TODO unless verbose is specified on the command line only do the checks and fail if invalid
#TODO what about start positions?
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

if ( ! getopts "hvpiGqofZLnNDwlyYmdagFtrz" opt)
then
    echo "$HELPSHORT"
fi
while getopts "hvpiGqo:fZLn:N:D:w:l:y:Y:m:d:a:g:F:t:r:z:" opt; do
    case $opt in
        h) echo "$HELP"; exit;;
        v) VERBOSE='y';;
        p) PROGRESS='y';;
        i) INTERACTIVE='y';;
        G) GAMEON='y';OUTPUT_LINK='y';;
        q) QUIET='y';;
        o) OUTPUT_PATH=$OPTARG;;
        f) OUTPUT_OVERWRITE='y';;
        Z) OUTPUT_COMPRESS='y';;
        L) OUTPUT_LINK='y';;
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
        F) MAP_FEATURES=$OPTARG;;
        t) MAP_TYPEIMAGE=$OPTARG;;
        r) MAP_METALIMAGE=$OPTARG;;
        z) MAP_WATERLEVEL=$OPTARG;;
    esac
done

echoq()
{
    if [[ $QUIET == 'n' ]];
    then
        echo -e "[makemap]$@"
    fi
}

echov()
{
    if [[ $VERBOSE == 'y' ]]
    then
        echoq $@
    fi
}

OptionAsk()
{
    # messed up shit to get array indirect referencing working
    temp="${1}[@]"
    OPTION=( "${!temp}" )

    echo -e "${OPTION[2]}"
    echo -e "${OPTION[3]}"
    read -re -p "${OPTION[1]}: " $1

    #regex match testing
    if [[ ! "${!1}" =~ ${OPTION[1]} ]]
    then
        echo \
"you entered: \"${!1}\" unfortunately that does not match the regex pattern,
please try again."
        OptionAsk $1
    fi
#TODO Validate input, this could be nested function calls with the validation
# on a different layer ie OptionAskImage
}


OptionsReview()
{
    echo -e \
"\nOptions Review:
================

General options:
  q:quiet                 : $QUIET
  v:verbose               : $VERBOSE
  p:progress              : $PROGRESS
  f:overwrite             : $OUTPUT_OVERWRITE
  o:output path           : $OUTPUT_PATH
  Z:compress              : $OUTPUT_COMPRESS
  L:link to maps folder   : $OUTPUT_LINK
  G:test map              : $GAMEON

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
  F:features file         : $MAP_FEATURES

  t:type image            : $MAP_TYPEIMAGE
  r:metal image           : $MAP_METALIMAGE"

}

ValidateOptions()
{
    RETVAL=0
    #test each option for validity and provide an error message if it aint so.
    if [[ ! $MAP_LONGNAME ]]
    then
        MAP_LONGNAME=$MAP_NAME
    fi

    if [[ ! $MAP_BREADTH || $MAP_BREADTH -lt 4 ]]; then
        echoq "w:map width='$MAP_BREADTH' invalid"
        RETVAL=1
    fi
    if [[ ! $MAP_LENGTH ]]; then
        echoq "l:map length='$MAP_LENGTH' invalid"
        RETVAL=1
    fi
    if [[ ! $MAP_DIFFUSEIMAGE ]]; then
        echoq "d:diffuse image='$MAP_DIFFUSEIMAGE' invalid"
        RETVAL=1
    fi

    if [[ $GAMEON == 'y' ]]; then OUTPUT_LINK='y';fi

    OUTPUT_PATH=`readlink -m "$OUTPUT_PATH"`
    return $RETVAL
}


echov 'Initial Validation Check'
ValidateOptions

# Review Options
# ==============
echov 'Interactive loop' $INTERACTIVE
while [[ "$INTERACTIVE" == 'y' ]]
do
    clear
    ValidateOptions
    VALID=$?
    if [[ $VALID -eq 1 ]]; then
        echo "Some requirements have not been satisfied";
    fi

    OptionsReview
    echo -e \
"\nPre-Processing Functions
  z:water level

  Q: Nooooo, get me out of here!!!
choose option, or press enter to confirm:"
    read RETVAL
    if [[ ! $RETVAL ]]
    then
        RETVAL=C
    fi

    case $RETVAL in
        q) OptionAsk QUIET;;
        v) OptionAsk VERBOSE;;
        p) OptionAsk PROGRESS;;
        f) OptionAsk OUTPUT_OVERWRITE;;
        o) OptionAsk OUTPUT_PATH;;
        Z) OptionAsk OUTPUT_COMPRESS;;
        L) OptionAsk OUTPUT_LINK;;
        G) OptionAsk GAMEON;;
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
        F) OptionAsk MAP_FEATURES;;
        t) OptionAsk MAP_TYPEIMAGE;;
        r) OptionAsk MAP_METALIMAGE;;
        z) OptionAsk MAP_WATERLEVEL;;
        Q) exit;;
        C) INTERACTIVE='n';;
    esac

    if [[ $VALID -eq 1 ]]; then
        INTERACTIVE=y;
    fi
done

# last minute validation to prevent bad options going to command lines
ValidateOptions
if [[ $? -ne 0 ]]; then
    echoq "option validation checks failed"
    exit 1
fi

#TODO ouput makemap cmdline now that all options are validated

if [[ $QUIET == 'n' ]]; then OptionsReview;fi

# create folder structure
# -----------------------
echoq 'creating Directory Structure'
BASE_PATH="${OUTPUT_PATH}/${MAP_NAME}.sdd"
mkdir -p "$BASE_PATH"/{maps,LuaGaia/Gadgets}
if [[ $? -ne 0 ]]
then
    echoq "Failed to create $BASE_PATH"
    exit 2
fi

# populate Feature Placer files
# -----------------------------
echoq 'Creating boilerplate files for featureplacer'

boilerplate()
{
    #$1 = filename
    #$2 = filecontent
    if [[ $OUTPUT_OVERWRITE == 'y' || ! -e "$1" ]]
    then
        echo "$2" > "$1"
        if [[ $? -ne 0 ]]
        then
            echoq "failed to create $1"
            exit 2
        fi
    else
        echoq "$1 exists, skippping"
    fi
}

# LuaGaia/main.lua
#TODO dont clobber if already existsa
FILENAME="$BASE_PATH/LuaGaia/main.lua"
FILECONTENT='if AllowUnsafeChanges then AllowUnsafeChanges("USE AT YOUR OWN PERIL") end
VFS.Include("LuaGadgets/gadgets.lua",nil, VFS.BASE)'
boilerplate "$FILENAME" "$FILECONTENT"

# LuaGaia/draw.lua
FILENAME="$BASE_PATH/LuaGaia/draw.lua"
FILECONTENT='VFS.Include("LuaGadgets/gadgets.lua",nil, VFS.BASE)'
boilerplate "$FILENAME" "$FILECONTENT"

# LuaGaia/Gadgets/featureplacer.lua
FILENAME="$BASE_PATH/LuaGaia/Gadgets/featureplacer.lua"
FILECONTENT='function gadget:GetInfo()
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

return false --unload'
boilerplate "$FILENAME" "$FILECONTENT"

# TODO generate this file from source material rather than an empty one
# features.lua
FILENAME="$BASE_PATH/features.lua"
FILECONTENT='local lists = {
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
return lists'
boilerplate "$FILENAME" "$FILECONTENT"

# Map Info
# ========
# mapinfo.lua
#FIXME homogenise depth/height/floor/ceiling/breadth/length variable and docuemntataion names
# as much as you dont like it use the spring game words, so go through the spring source
FILENAME="$BASE_PATH/mapinfo.lua"
FILECONTENT="--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- mapinfo.lua
--

local mapinfo = {
    name        = '$MAP_LONGNAME',
    shortname   = '$MAP_NAME',
    description = '$MAP_DESCRIPTION',
    author      = '${USER:-`whoami`}',
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
        minheight = ${MAP_FLOOR:-0},
        maxheight = ${MAP_CEILING:-0},
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

return mapinfo"
boilerplate "$FILENAME" "$FILECONTENT"

# Run the smt_convert command to create tile file from images
# -----------------------------------------------------------
echoq 'Generating the smt file'

COMMAND='smt_convert'
if [[ $VERBOSE == 'y' ]]; then COMMAND+=' -v'; fi
if [[ $PROGRESS == 'y' ]]; then COMMAND+=' -p'; fi
if [[ $QUIET == 'y' ]]; then COMMAND+=' -q'; fi
if [[ $OUTPUT_OVERWRITE == 'y' ]]; then COMMAND+=' -f'; fi
COMMAND+=" --imagesize $((512 * $MAP_BREADTH))x$((512 * $MAP_LENGTH))"
COMMAND+=' --smt --tilesize 32x32 --type DXT1'
COMMAND+=" -o \"$BASE_PATH/maps/${MAP_NAME}.smt\""
COMMAND+=" $MAP_DIFFUSEIMAGE"

echoq "$COMMAND"
eval "$COMMAND"
if [[ $? -ne 0 ]]; then exit 2; fi

# run the smf_cc command to create the smf file
# ---------------------------------------------
echoq 'Generating the smf file'
COMMAND='smf_cc'
if [[ $VERBOSE == 'y' ]]; then COMMAND+=' -v'; fi
#if [[ $PROGRESS == 'y' ]]; then COMMAND+=' -p'; fi # maybe add a progress indicator to smf_cc?
if [[ $QUIET == 'y' ]]; then COMMAND+=' -q'; fi
if [[ $OUTPUT_OVERWRITE -eq 'y' ]]; then COMMAND+=' -f'; fi
COMMAND+=" --mapsize ${MAP_BREADTH}x${MAP_LENGTH}"
COMMAND+=' --tilesize 32'
if [[ $MAP_FLOOR ]]; then COMMAND+=" -y $MAP_FLOOR";fi
if [[ $MAP_CEILING ]]; then COMMAND+=" -Y $MAP_CEILING";fi
COMMAND+=" --tilemap \"$BASE_PATH/maps/$MAP_NAME.smt.csv\""
if [[ $MAP_HEIGHTIMAGE ]]; then COMMAND+=" --height $MAP_HEIGHTIMAGE"; fi
if [[ $MAP_TYPEIMAGE ]]; then COMMAND+=" --type $MAP_TYPEIMAGE"; fi
if [[ $MAP_MINIIMAGE ]]; then COMMAND+=" --mini $MAP_MINIIMAGE"; fi
if [[ $MAP_METALIMAGE ]]; then COMMAND+=" --metal $MAP_METALIMAGE"; fi
if [[ $MAP_GRASSIMAGE ]]; then COMMAND+=" --grass $MAP_GRASSIMAGE"; fi
COMMAND+=" -o \"$BASE_PATH/maps/${MAP_NAME}.smf\""
COMMAND+=" \"$BASE_PATH/maps/$MAP_NAME.smt\""

echoq "$COMMAND"
eval "${COMMAND}"
if [[ $? -ne 0 ]]; then exit 2; fi

# delete superflous tilefile
# --------------------------
rm "$BASE_PATH/maps/${MAP_NAME}.smt.csv"
if [[ $? -ne 0 ]]; then exit 2; fi

# zip up the files into a 7z map archive
# ------------------------------------
#TODO error messages, feedback etc
if [[ $OUTPUT_COMPRESS == 'y' ]]
then
    echoq 'Compressing contents into 7z map archive'
    FILENAME="${MAP_NAME}.sd7"
    FILEPATH="$OUTPUT_PATH/$FILENAME"
    if [[ -e "$FILEPATH" && $OUTPUT_OVERWRITE == 'y' ]]
    then rm "$FILEPATH"; fi
    if [[ -e "$FILEPATH" && $OUTPUT_OVERWRITE == 'n' ]];
    then
        echoq "Cannot overwrite $FILEPATH, please specify -f"
    fi
    if [[ ! -e "$FILEPATH" ]]
    then
        CURRENT_DIR=`pwd`
        COMMAND='7z a -ms=off -mx=9'
        if [[ $VERBOSE == 'y' ]]; then COMMAND+=' -bt -bse2 -bso2 -bb3';fi
        if [[ $PROGRESS == 'n' ]]; then COMMAND+=' -bd';fi
        if [[ $QUIET == 'y' ]]; then COMMAND+=' -bse0 -bso0';fi
        COMMAND+=" $FILENAME *"

        echoq $COMMAND
        cd "$BASE_PATH"
        eval "$COMMAND"
        if [[ $? -gt 1 ]]
        then
            echoq 'failure to compress archive' $FILENAME
            exit 2; fi
        mv "$FILENAME" "$OUTPUT_PATH/"
        cd "$CURRENT_DIR"

    fi
fi

# Link the generated game folder into the spring settings folder
# --------------------------------------------------------------
#FIXME get the spring map folder directly from the spring settings
if [[ $OUTPUT_LINK == 'y' ]]
then
    echoq 'Linking generated map archive into spring config folders'
    COMMAND='ln -s'
    if [[ $OUTPUT_OVERWRITE == 'y' ]]; then COMMAND+='f'; fi
    #TODO get spring maps folder from spring configuration file
    COMMAND+=" \"$OUTPUT_PATH/${MAP_NAME}.sdd\" \"/home/$USER/.config/spring/maps\""
    echoq "$COMMAND"
    eval "$COMMAND"
    if [[ $? -ne 0 ]]; then exit 2; fi
fi

# Run the Game to test the map
# ----------------------------
#TODO clean up this script to be minimal
if [[ $GAMEON == 'y' ]]; then
    echo \
"[game]
{
[allyteam0]
{
numallies=0;
}
[mapoptions]
{
}
[modoptions]
{
disablemapdamage=0;
fixedallies=0;
ghostedbuildings=1;
limitdgun=0;
maxspeed=3;
maxunits=500;
minspeed=0.3;
relayhoststartpostype=3;
}
[player0]
{
countrycode=;
isfromdemo=0;
name=Player;
rank=0;
spectator=0;
team=0;
}
[restrict]
{
}
[team0]
{
allyteam=0;
handicap=0;
rgbcolor=1 1 0;
side=TANKS;
startposx=128;
startposz=128;
teamleader=0;
}
gametype=Empty Mod;
hostip=;
hostport=8452;
ishost=1;
maphash=1766029540;
mapname=$MAP_NAME;
modhash=1226406912;
myplayername=Player;
numplayers=1;
numrestrictions=0;
numusers=1;
startpostype=3;
}
" > script.txt
    spring script.txt | grep $MAP_NAME
    RETVAL=$?
    rm script.txt
fi
# spring always crashes on my system with a core dump :|
if [[ $RETVAL -eq 255 ]]; then exit 2; fi

# Remaining Items on my thought list
# ----------------------------------
#TODO systematically change double qotes " to single quotes ' if variable substitution is not nexessary
#TODO validate input mapsize to be multiples of two, and update the help text to specify it.
#TODO validate input images

#TODO for lighting give presets like sunset, sunrise, midday, dusk, night etc..
#TODO automatically detect sizes of things based on image dimensions

#Future maybe
# optionally create mapinfo/* files depending on options. but i think i might remove them in this version
# give presets for weather effects, like light rain, heavy rain, snow, windy etc.
# TODO use some form of toolkit to allow file selection using GUI
# NOTE yad is a good option to use for dialog based formsw
