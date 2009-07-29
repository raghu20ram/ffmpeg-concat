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

/** @file libavformat/playlist.c
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief General components used by playlist formats
 *
 *  @details These functions are used to initialize and manipulate playlists
 *  (PlaylistContext) and their individual playlist elements (PlayElem), each
 *  of which encapsulates its own AVFormatContext. This abstraction is used for
 *  implementing file concatenation and support for playlist formats.
 */

#include "avformat.h"
#include "playlist.h"
#include "internal.h"

AVFormatContext *ff_playlist_alloc_formatcontext(char *filename)
{
    int err;
    AVFormatContext *ic = av_mallocz(sizeof(*ic));
    err = av_open_input_file(&(ic), filename, ic->iformat, 0, NULL);
    if (err < 0)
        av_log(ic, AV_LOG_ERROR, "Error during av_open_input_file\n");
    err = av_find_stream_info(ic);
    if (err < 0)
        av_log(ic, AV_LOG_ERROR, "Could not find stream info\n");
    return ic;
}

void ff_playlist_populate_context(AVFormatContext *s)
{
    int i;
    AVFormatContext *ic;
    PlaylistContext *ctx = s->priv_data;
    ctx->icl = av_realloc(ctx->icl, sizeof(*(ctx->icl)) * (ctx->pe_curidx+2));
    ctx->icl[ctx->pe_curidx+1] = NULL;
    ic = ctx->icl[ctx->pe_curidx] = ff_playlist_alloc_formatcontext(ctx->flist[ctx->pe_curidx]);
    ic->iformat->read_header(ic, 0);
    s->nb_streams = ic->nb_streams;
    for (i = 0; i < ic->nb_streams; ++i)
        s->streams[i] = ic->streams[i];
    s->packet_buffer = ic->packet_buffer;
    s->packet_buffer_end = ic->packet_buffer_end;
}

PlaylistContext *ff_playlist_get_context(AVFormatContext *ic)
{
    if (ic && ic->iformat && ic->iformat->long_name && ic->priv_data &&
        !strncmp(ic->iformat->long_name, "CONCAT", 6))
        return ic->priv_data;
    else
        return NULL;
}

void ff_playlist_set_context(AVFormatContext *ic, PlaylistContext *ctx)
{
    if (ic && ctx)
        ic->priv_data = ctx;
}

AVStream *ff_playlist_get_stream(PlaylistContext *ctx, int pe_idx, int stream_index)
{
    if (ctx && pe_idx < ctx->pelist_size && ctx->icl && ctx->icl[pe_idx] &&
        ctx->icl[pe_idx] && stream_index < ctx->icl[pe_idx]->nb_streams &&
        ctx->icl[pe_idx]->streams && ctx->icl[pe_idx]->streams[stream_index])
        return ctx->icl[pe_idx]->streams[stream_index];
    else
        return NULL;
}

void ff_playlist_split_encodedstring(char *s, char sep, char ***flist_ptr, int *len_ptr)
{
    char c, *ts, **flist;
    int i, len, buflen, *sepidx;
    buflen = sepidx = len = 0;
    sepidx = av_fast_realloc(sepidx, &buflen, ++len);
    sepidx[0] = 0;
    ts = s;
    while ((c = *ts++) != 0) {
        if (c == sep) {
            sepidx[len] = ts-s;
            sepidx = av_fast_realloc(sepidx, &buflen, ++len);
        }
    }
    sepidx[len] = ts-s;
    ts = s;
    *len_ptr = len;
    *flist_ptr = flist = av_malloc(sizeof(*flist) * (len+1));
    flist[len] = 0;
    for (i = 0; i < len; ++i) {
        flist[i] = av_malloc(sepidx[i+1]-sepidx[i]);
        av_strlcpy(flist[i], ts+sepidx[i], sepidx[i+1]-sepidx[i]);
    }
    av_free(sepidx);
}

PlaylistContext *ff_playlist_from_encodedstring(char *s, char sep)
{
    PlaylistContext *ctx;
    char **flist;
    int i, len;
    ff_playlist_split_encodedstring(s, sep, &flist, &len);
    if (len <= 1) {
        for (i = 0; i < len; ++i)
            av_free(flist[i]);
        av_free(flist);
        return NULL;
    }
    ctx = av_mallocz(sizeof(*ctx));
    for (i = 0; i < len; ++i)
        ff_playlist_add_path(ctx, flist[i]);
    return ctx;
}

void ff_playlist_add_path(PlaylistContext *ctx, char *itempath)
{
    ctx->flist = av_realloc(ctx->flist, sizeof(*(ctx->flist)) * (++ctx->pelist_size+1));
    ctx->flist[ctx->pelist_size] = NULL;
    ctx->flist[ctx->pelist_size-1] = itempath;
}

// converts list of mixed absolute and relative paths into all absolute paths
void ff_playlist_relative_paths(char **flist, int len, const char *workingdir)
{
    int i;
    for (i = 0; i < len; ++i) { // determine if relative paths
        char *fullfpath;
        int wdslen = strlen(workingdir);
        int flslen = strlen(flist[i]);
        fullfpath = av_malloc(sizeof(char) * (wdslen+flslen+2));
        av_strlcpy(fullfpath, workingdir, wdslen+1);
        fullfpath[wdslen] = '/';
        fullfpath[wdslen+1] = 0;
        av_strlcat(fullfpath, flist[i], wdslen+flslen+2);
        if (url_exist(fullfpath))
            flist[i] = fullfpath;
    }
}

int ff_playlist_stream_index_from_time(PlaylistContext *ctx, int64_t pts)
{
    int i = 0;
    while (i < ctx->pelist_size) {
        if (ctx->icl[i]->cur_st->duration > pts) {
            pts -= ctx->icl[i]->cur_st->duration;
            ++i;
            if (!ctx->icl[i])
                ff_playlist_populate_context(ctx);
        }
    }
    return i;
}
