/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#pragma once

#include "various/Socket.h"
#include "backend/radio-receiver.h"
#include <lame/lame.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <chrono>
#include <string>

class ProgrammeSender {
    private:
        Socket s;

        bool running = true;
        std::condition_variable cv;
        std::mutex mutex;

    public:
        ProgrammeSender(Socket&& s);
        bool send_mp3(const std::vector<uint8_t>& mp3data);
        void wait_for_termination();
        void cancel();
};


struct Lame {
    lame_t lame;

    Lame() {
        lame = lame_init();
    }

    Lame(const Lame& other) = delete;
    Lame& operator=(const Lame& other) = delete;
    Lame(Lame&& other) = default;
    Lame& operator=(Lame&& other) = default;

    ~Lame() {
        lame_close(lame);
    }
};

enum class MOTType { JPEG, PNG, Unknown };


class WebProgrammeHandler : public ProgrammeHandlerInterface {
    public:
        struct xpad_error_t {
            bool has_error = false;
            size_t announced_xpad_len = 0;
            size_t xpad_len = 0;
            std::chrono::time_point<std::chrono::system_clock> time; };
    private:
        uint32_t serviceId;

        bool lame_initialised = false;
        Lame lame;

        int last_frameErrors = -1;
        int last_rsErrors = -1;
        int last_aacErrors = -1;

        mutable std::mutex senders_mutex;
        std::list<ProgrammeSender*> senders;

        mutable std::mutex pad_mutex;

        bool last_label_valid = false;
        std::chrono::time_point<std::chrono::system_clock> time_label;
        std::string last_label;

        bool last_mot_valid = false;
        std::chrono::time_point<std::chrono::system_clock> time_mot;
        std::vector<uint8_t> last_mot;
        MOTType last_subtype = MOTType::Unknown;

        xpad_error_t xpad_error;

    public:
        bool stereo = false;
        int rate = 0;
        std::string mode;
        int last_audioLevel_L = -1;
        int last_audioLevel_R = -1;

        WebProgrammeHandler(uint32_t serviceId);
        WebProgrammeHandler(WebProgrammeHandler&& other);

        void registerSender(ProgrammeSender *sender);
        void removeSender(ProgrammeSender *sender);
        bool needsToBeDecoded() const;
        void cancelAll();

        struct dls_t {
            std::string label;
            std::chrono::time_point<std::chrono::system_clock> time; };
        dls_t getDLS() const;

        struct mot_t {
            std::string data;
            MOTType subtype = MOTType::Unknown;
            std::chrono::time_point<std::chrono::system_clock> time; };
        mot_t getMOT_base64() const;

        xpad_error_t getXPADErrors() const;

        virtual void onFrameErrors(int frameErrors) override;
        virtual void onNewAudio(std::vector<int16_t>&& audioData,
                int sampleRate, bool isStereo, const std::string& mode) override;
        virtual void onRsErrors(int rsErrors) override;
        virtual void onAacErrors(int aacErrors) override;
        virtual void onNewDynamicLabel(const std::string& label) override;
        virtual void onMOT(const std::vector<uint8_t>& data, int subtype) override;
        virtual void onPADLengthError(size_t announced_xpad_len, size_t xpad_len) override;
};

