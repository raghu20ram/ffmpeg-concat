/*
 * General components used by playlist formats
 * Copyright (c) 2009 Geza Kovacs
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFORMAT_PLAYLIST_H
#define AVFORMAT_PLAYLIST_H

#include "avformat.h"
#include "riff.h"

/** @struct PlayElem
 *  @brief Represents each input file on a playlist
 */

typedef struct PlayElem {
    AVFormatContext *ic; /**< AVFormatContext for this playlist item */
    char *filename; /**< Filename with absolute path of this playlist item */
    AVInputFormat *fmt; /**< AVInputFormat manually specified for this playlist item */
    AVFormatParameters *ap; /**< AVFormatParameters for this playlist item */
} PlayElem;

/** @struct PlaylistContext
 *  @brief Represents the playlist and contains PlayElem for each playlist item
 */

typedef struct PlaylistContext {
    PlayElem **pelist; /**< List of PlayElem, with each representing a playlist item */
    int pelist_size; /**< Length of the pelist array (number of playlist items) */
    int *pe_curidxs; /**< Index of the PlayElem that each multimedia stream (video and audio) is currently on */
    int pe_curidxs_size; /**< Length of pe_curidxs array (number of multimedia streams) currently set to 2 (video and audio) */
    AVChapter **chlist; /**< List of chapters, with each playlist element representing a chapter */
    char *workingdir; /**< Directory in which the playlist file is stored in */
    char *filename; /**< Filename (not path) of the playlist file */
    int64_t *time_offsets; /**< Time offsets, in 10^-6 seconds, for each multimedia stream */
} PlaylistContext;

void ff_playlist_init_playelem(PlayElem* pe);

PlaylistContext* ff_playlist_alloc_context(const char *filename);

int ff_playlist_populate_context(PlaylistContext *playlc, AVFormatContext *s, int stream_index);

void ff_split_wd_fn(const char *filepath, char **workingdir, char **filename);

int64_t ff_playlist_get_duration(AVFormatContext *ic, int stream_index);

void ff_playlist_relative_paths(char **flist, const char *workingdir);

PlaylistContext* ff_playlist_get_context(AVFormatContext *ic);

AVStream *ff_playlist_get_stream(PlaylistContext *ctx, int pe_idx, int stream_index);

#endif /* AVFORMAT_PLAYLIST_H */
