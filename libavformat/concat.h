/*
 * Minimal playlist/concatenation demuxer
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

/** @file libavformat/concat.h
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief Minimal playlist/concatenation demuxer
 *
 *  @details This is a minimal concat-type demuxer to which elements can be
 *  added externally via av_playlist_add_filelist.
 */

#ifndef AVFORMAT_CONCAT_H
#define AVFORMAT_CONCAT_H

#include "concatgen.h"

AVInputFormat* ff_concat_alloc_demuxer(void);

#endif /* AVFORMAT_CONCAT_H */

