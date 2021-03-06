﻿#include "rtc_media_stream_impl.h"
#include "rtc_audio_track_impl.h"
#include "rtc_peerconnection.h"
#include "rtc_video_track_impl.h"

#include <algorithm>

namespace libwebrtc {

MediaStreamImpl::MediaStreamImpl(
    rtc::scoped_refptr<webrtc::MediaStreamInterface> rtc_media_stream)
    : rtc_media_stream_(rtc_media_stream) {
  rtc_media_stream_->RegisterObserver(this);

  for (auto track : rtc_media_stream->GetAudioTracks()) {
    scoped_refptr<AudioTrackImpl> audio_track = scoped_refptr<AudioTrackImpl>(
        new RefCountedObject<AudioTrackImpl>(track));
    audio_tracks_.push_back(audio_track);
  }

  for (auto track : rtc_media_stream->GetVideoTracks()) {
    scoped_refptr<VideoTrackImpl> video_track = scoped_refptr<VideoTrackImpl>(
        new RefCountedObject<VideoTrackImpl>(track));
    video_tracks_.push_back(video_track);
  }

  strncpy(label_, rtc_media_stream_->id().c_str(), sizeof(label_));
}

MediaStreamImpl::~MediaStreamImpl() {
  RTC_LOG(INFO) << __FUNCTION__ << ": dtor ";
  rtc_media_stream_->UnregisterObserver(this);
  audio_tracks_.clear();
  video_tracks_.clear();
}

bool MediaStreamImpl::AddTrack(scoped_refptr<RTCAudioTrack> track) {
  AudioTrackImpl* track_impl = static_cast<AudioTrackImpl*>(track.get());
  if (rtc_media_stream_->AddTrack(track_impl->rtc_track())) {
    audio_tracks_.push_back(track);
    return true;
  }
  return false;
}

bool MediaStreamImpl::AddTrack(scoped_refptr<RTCVideoTrack> track) {
  VideoTrackImpl* track_impl = static_cast<VideoTrackImpl*>(track.get());
  if (rtc_media_stream_->AddTrack(track_impl->rtc_track())) {
    video_tracks_.push_back(track);
    return true;
  }
  return false;
}

bool MediaStreamImpl::RemoveTrack(scoped_refptr<RTCAudioTrack> track) {
  AudioTrackImpl* track_impl = static_cast<AudioTrackImpl*>(track.get());
  if (rtc_media_stream_->RemoveTrack(track_impl->rtc_track())) {
    auto it = std::find(audio_tracks_.begin(), audio_tracks_.end(), track);
    if (it != audio_tracks_.end())
      audio_tracks_.erase(it);
    return true;
  }
  return false;
}

bool MediaStreamImpl::RemoveTrack(scoped_refptr<RTCVideoTrack> track) {
  VideoTrackImpl* track_impl = static_cast<VideoTrackImpl*>(track.get());
  if (rtc_media_stream_->RemoveTrack(track_impl->rtc_track())) {
    auto it = std::find(video_tracks_.begin(), video_tracks_.end(), track);
    if (it != video_tracks_.end())
      video_tracks_.erase(it);
    return true;
  }
  return false;
}

AudioTrackVector MediaStreamImpl::GetAudioTracks() {
  return audio_tracks_;
}

VideoTrackVector MediaStreamImpl::GetVideoTracks() {
  return video_tracks_;
}

scoped_refptr<RTCAudioTrack> MediaStreamImpl::FindAudioTrack(
    const char* track_id) {
  for (auto track : audio_tracks_) {
    if (strcmp(track->id(), track_id) == 0)
      return track;
  }

  return nullptr;
}

scoped_refptr<RTCVideoTrack> MediaStreamImpl::FindVideoTrack(
    const char* track_id) {
  for (auto track : video_tracks_) {
    if (strcmp(track->id(), track_id) == 0)
      return track;
  }

  return nullptr;
}

void MediaStreamImpl::OnChanged() {
  AudioTrackVector audio_tracks;
  for (auto track : rtc_media_stream_->GetAudioTracks()) {
    scoped_refptr<AudioTrackImpl> audio_track = scoped_refptr<AudioTrackImpl>(
        new RefCountedObject<AudioTrackImpl>(track));
    audio_tracks.push_back(audio_track);
  }

  audio_tracks_ = audio_tracks;

  VideoTrackVector video_tracks;
  for (auto track : rtc_media_stream_->GetVideoTracks()) {
    scoped_refptr<VideoTrackImpl> video_track = scoped_refptr<VideoTrackImpl>(
        new RefCountedObject<VideoTrackImpl>(track));
    video_tracks.push_back(video_track);
  }

  /*对比流，回调OnAddTrack 或 OnRemoveTrack*/
  VideoTrackVector removed_video_tracks;

  for (auto track : video_tracks_) {
    if (std::find(video_tracks.begin(), video_tracks.end(), track) ==
        video_tracks.end()) {
      removed_video_tracks.push_back(track);
    }
  }

  for (auto track : removed_video_tracks) {
    if (observer_)
      observer_->OnRemoveTrack(this, track);
  }

  VideoTrackVector new_video_tracks;
  for (auto track : video_tracks) {
    if (std::find(video_tracks_.begin(), video_tracks_.end(), track) ==
        video_tracks_.end()) {
      new_video_tracks.push_back(track);
    }
  }

  for (auto track : new_video_tracks) {
    if (observer_)
      observer_->OnAddTrack(this, track);
  }

  video_tracks_ = video_tracks;
}

} // namespace libwebrtc
