// sample_group.hxx -- Manage a group of samples relative to a base position
//
// Written for the new SoundSystem by Erik Hofman, October 2009
//
// Copyright (C) 2009 Erik Hofman <erik@ehofman.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

/**
 * \file sample_group.hxx
 * sample groups contain all sounds related to one specific object and
 * have to be added to the sound manager, otherwise they won't get processed.
 */

#ifndef _SG_SAMPLE_GROUP_OPENAL_HXX
#define _SG_SAMPLE_GROUP_OPENAL_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#if defined(__APPLE__)
# include <OpenAL/al.h>
#elif defined(OPENALSDK)
# include <al.h>
#else
# include <AL/al.h>
#endif

#include <string>
#include <vector>
#include <map>

#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/exception.hxx>

#include "sample_openal.hxx"

using std::map;
using std::string;

typedef map < string, SGSharedPtr<SGSoundSample> > sample_map;
typedef sample_map::iterator sample_map_iterator;
typedef sample_map::const_iterator const_sample_map_iterator;

class SGSoundMgr;

class SGSampleGroup : public SGReferenced
{
public:

    /**
     * Empty constructor for class encapsulation.
     * Note: The sample group should still be activated before use
     */
    SGSampleGroup ();

    /**
     * Constructor
     * Register this sample group at the sound manager using refname
     * Note: The sample group should still be activated before use
     * @param smgr Pointer to a pre-initialized sound manager class
     * @param refname Name of this group for reference purposes.
     */
    SGSampleGroup ( SGSoundMgr *smgr, const string &refname );

    /**
     * Destructor
     */
    ~SGSampleGroup ();

    /**
     * Set the status of this sample group to active.
     */
    inline void activate() { _active = true; }

    /**
     * Update function.
     * Call this function periodically to update the OpenAL state of all
     * samples associated with this class. None op the configuration changes
     * take place without a call to this function.
     */
    virtual void update (double dt);

    /**
     * Register an audio sample to this group.
     * @param sound Pointer to a pre-initialized audio sample
     * @param refname Name of this audio sample for reference purposes
     * @return return true if successful
     */
    bool add( SGSharedPtr<SGSoundSample> sound, const string& refname );

    /**
     * Remove an audio sample from this group.
     * @param refname Reference name of the audio sample to remove
     * @return return true if successful
     */
    bool remove( const string& refname );

    /**
     * Test if a specified audio sample is registered at this sample group
     * @param refname Reference name of the audio sample to test for
     * @return true if the specified audio sample exists
     */
    bool exists( const string& refname );

    /**
     * Find a specified audio sample in this sample group
     * @param refname Reference name of the audio sample to find
     * @return A pointer to the SGSoundSample
     */
    SGSoundSample *find( const string& refname );

    /**
     * Request to stop playing all audio samples until further notice.
     */
    void suspend();

    /**
     * Request to resume playing all audio samples.
     */
    void resume();

    /**
     * Request to start playing the refered audio sample.
     * @param refname Reference name of the audio sample to start playing
     * @param looping Define if the sound should loop continuously
     * @return true if the audio sample exsists and is scheduled for playing
     */
    bool play( const string& refname, bool looping );
    
    /**
     * Request to start playing the refered audio sample looping.
     * @param refname Reference name of the audio sample to start playing
     * @return true if the audio sample exsists and is scheduled for playing
     */
    inline bool play_looped( const string& refname ) {
        return play( refname, true );
    }

    /**
     * Request to start playing the refered audio sample once.
     * @param refname Reference name of the audio sample to start playing
     * @return true if the audio sample exsists and is scheduled for playing
     */
    inline bool play_once( const string& refname ) {
        return play( refname, false );
    }

    /**
     * Test if a referenced sample is playing or not.
     * @param refname Reference name of the audio sample to test for
     * @return True of the specified sound is currently playing
     */
    bool is_playing( const string& refname );

    /**
     * Request to stop playing the refered audio sample.
     * @param refname Reference name of the audio sample to stop
     * @return true if the audio sample exsists and is scheduled to stop
     */
    bool stop( const string& refname );

    /**
     * Set the master volume for this sample group.
     * @param vol Volume (must be between 0.0 and 1.0)
     */
    void set_volume( float vol );

    /**
     * Set the velocity vector of this sample group.
     * This is in the local frame coordinate system; x=north, y=east, z=down
     * @param vel Velocity vector 
     */
    void set_velocity( const SGVec3d& vel ) {
       _velocity = vel; _changed = true;
    }

    /**
     * Set the position of this sample group.
     * This is in the same coordinate system as OpenGL; y=up, z=back, x=right.
     * @param pos Base position
     */
    void set_position_geod( const SGGeod& pos ) {
        _base_pos = pos; _changed = true;
    }

    /**
     * Set the orientation of this sample group.
     * @param ori Quaternation containing the orientation information
     */
    void set_orientation( const SGQuatd& ori ) {
        _orientation = ori; _changed = true;
    }

    /**
     * Tie this sample group to the listener position, orientation and velocity
     */
    void tie_to_listener() { _tied_to_listener = true; }

protected:
    SGSoundMgr *_smgr;
    string _refname;
    bool _active;

private:
    bool _changed;
    bool _pause;
    float _volume;
    bool _tied_to_listener;

    SGVec3d _velocity;
    SGGeod _base_pos;
    SGQuatd _orientation;

    sample_map _samples;
    std::vector< SGSharedPtr<SGSoundSample> > _removed_samples;

    bool testForALError(string s);
    bool testForError(void *p, string s);

    void update_pos_and_orientation();
    void update_sample_config( SGSoundSample *sound );
};

#endif // _SG_SAMPLE_GROUP_OPENAL_HXX
