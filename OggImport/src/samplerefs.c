/*
 *  samplerefs.c
 *
 *    SampleReference arrays handling utilities.
 *
 *
 *  Copyright (c) 2007  Arek Korbik
 *
 *  This file is part of XiphQT, the Xiph QuickTime Components.
 *
 *  XiphQT is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  XiphQT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id: samplerefs.c 12754 2007-03-14 03:51:23Z arek $
 *
 */


#include "samplerefs.h"
#include "debug.h"

ComponentResult _store_sample_reference(StreamInfo *si, SInt64 *dataOffset, int size, TimeValue duration, short smp_flags)
{
    ComponentResult err = noErr;
    SampleReference64Record *srptr = NULL;

    if (si->sample_refs_count >= si->sample_refs_size) {
        //resize the sample_refs array
        si->sample_refs_size += si->sample_refs_increment;
        srptr = realloc(si->sample_refs, si->sample_refs_size * sizeof(SampleReference64Record));
        if (srptr == NULL)
            err = MemError();
        else
            si->sample_refs = srptr;
    }

    if (!err) {
        srptr = &si->sample_refs[si->sample_refs_count];
        memset(srptr, 0, sizeof(SampleReference64Record));
        srptr->dataOffset = SInt64ToWide(*dataOffset);
        srptr->dataSize = size;
        srptr->durationPerSample = duration;
        srptr->numberOfSamples = 1;
        srptr->sampleFlags = smp_flags;

        dbg_printf("[OIsr]  ++ _store_sample_reference() sampleRef: %lld, len: %d, dur: %d [%08lx, %08lx]\n", *dataOffset, size, duration,
                   (UInt32) si->sample_refs, (UInt32) srptr);

        si->sample_refs_count += 1;
        si->sample_refs_duration += duration;
    }

    return err;
}

ComponentResult _commit_srefs(OggImportGlobals *globals, StreamInfo *si, Boolean *movie_changed)
{
    ComponentResult err = noErr;
    TimeValue inserted  = 0;

    TimeValue movieTS = GetMovieTimeScale(globals->theMovie);
    TimeValue mediaTS = 0;
    Float64 mediaTS_fl = 0.0;

    if (si->sample_refs_count < 1) {
        *movie_changed = false;
        return err;
    }

    dbg_printf("[OIsr]  +> _commit_srefs() sampleRefs: %lld, count: %ld, dur: %ld\n", globals->dataOffset, si->sample_refs_count,
               si->sample_refs_duration);
    err = AddMediaSampleReferences64(si->theMedia, si->sampleDesc, si->sample_refs_count, si->sample_refs, &inserted);


    if (!err) {
        TimeValue timeLoaded;
        Float64 timeLoadedSubSecond;

        si->mediaLength += si->sample_refs_duration;

        err = InsertMediaIntoTrack(si->theTrack, si->insertTime, inserted,
                                   si->sample_refs_duration, fixed1);
        if (si->insertTime == 0) {
            if (si->streamOffset != 0) {
                SetTrackOffset(si->theTrack, si->streamOffset);
                dbg_printf("   # -- SetTrackOffset(%ld) = %ld --> %ld\n",
                           si->streamOffset, GetMoviesError(),
                           GetTrackOffset(si->theTrack));
                if (globals->dataIsStream) {
                    SetTrackEnabled(si->theTrack, false);
                    SetTrackEnabled(si->theTrack, true);
                }
            }
        }
        si->insertTime = -1;

        mediaTS = GetMediaTimeScale(si->theMedia);
        mediaTS_fl = (Float64) mediaTS;
        timeLoaded = si->streamOffset + si->mediaLength / mediaTS * movieTS + (si->mediaLength % mediaTS) * movieTS / mediaTS;
        timeLoadedSubSecond = (Float64) ((si->streamOffset % movieTS * mediaTS / movieTS + si->mediaLength) % mediaTS) / mediaTS_fl;

        if (globals->timeLoaded < timeLoaded || (globals->timeLoaded == timeLoaded && globals->timeLoadedSubSecond < timeLoadedSubSecond)) {
            globals->timeLoaded = timeLoaded;
            globals->timeLoadedSubSecond = timeLoadedSubSecond;
        }

        *movie_changed = true;

        si->sample_refs_duration = 0;
        si->sample_refs_count = 0;
    }

    return err;
}
