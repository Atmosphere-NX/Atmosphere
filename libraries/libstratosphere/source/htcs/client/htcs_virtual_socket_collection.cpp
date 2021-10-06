/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stratosphere.hpp>
#include "htcs_session.hpp"
#include "htcs_virtual_socket_collection.hpp"

namespace ams::htcs::client {

    namespace {

        constexpr inline s32 InvalidSocket = -1;
        constexpr inline s32 InvalidPrimitive = -1;

    }

    /* Declare client functions. */
    sf::SharedPointer<tma::ISocket> socket(s32 &last_error);
    s32 close(sf::SharedPointer<tma::ISocket> socket, s32 &last_error);
    s32 bind(sf::SharedPointer<tma::ISocket> socket, const htcs::SockAddrHtcs *address, s32 &last_error);
    s32 listen(sf::SharedPointer<tma::ISocket> socket, s32 backlog_count, s32 &last_error);
    sf::SharedPointer<tma::ISocket> accept(sf::SharedPointer<tma::ISocket> socket, htcs::SockAddrHtcs *address, s32 &last_error);
    s32 fcntl(sf::SharedPointer<tma::ISocket> socket, s32 command, s32 value, s32 &last_error);

    s32 shutdown(sf::SharedPointer<tma::ISocket> socket, s32 how, s32 &last_error);

    ssize_t recv(sf::SharedPointer<tma::ISocket> socket, void *buffer, size_t buffer_size, s32 flags, s32 &last_error);
    ssize_t send(sf::SharedPointer<tma::ISocket> socket, const void *buffer, size_t buffer_size, s32 flags, s32 &last_error);

    s32 connect(sf::SharedPointer<tma::ISocket> socket, const htcs::SockAddrHtcs *address, s32 &last_error);

    s32 select(s32 * const read, s32 &num_read, s32 * const write, s32 &num_write, s32 * const except, s32 &num_except, htcs::TimeVal *timeout, s32 &last_error);

    struct VirtualSocket {
        s32 m_id;
        s32 m_primitive;
        sf::SharedPointer<tma::ISocket> m_socket;
        bool m_do_bind;
        htcs::SockAddrHtcs m_address;
        s32 m_listen_backlog_count;
        s32 m_fcntl_command;
        s32 m_fcntl_value;
        bool m_blocking;


        VirtualSocket() {
            /* Initialize. */
            this->Init();
        }

        ~VirtualSocket() { /* ... */ }

        void Init() {
            /* Setup fields. */
            m_id        = InvalidSocket;
            m_primitive = InvalidPrimitive;
            m_socket    = nullptr;
            m_blocking  = true;
            m_do_bind   = false;

            std::memset(std::addressof(m_address), 0, sizeof(m_address));

            m_listen_backlog_count = -1;

            m_fcntl_command = -1;
            m_fcntl_value   = 0;
        }

        s32 Bind(const htcs::SockAddrHtcs *address, s32 &last_error) {
            /* Mark the bind. */
            m_do_bind = true;

            /* Set our address. */
            std::memcpy(std::addressof(m_address), address, sizeof(m_address));

            /* Clear the error. */
            last_error = 0;
            return 0;
        }

        s32 Listen(s32 backlog_count, s32 &last_error) {
            s32 res = -1;
            if (m_do_bind) {
                /* Set backlog count. */
                m_listen_backlog_count = std::max(backlog_count, 1);

                /* Clear error. */
                last_error = 0;
                res        = 0;
            } else {
                last_error = HTCS_EINVAL;
            }
            return res;
        }

        s32 Fcntl(s32 command, s32 value, s32 &last_error) {
            /* Clear error. */
            s32 res    = 0;
            last_error = 0;

            if (command == HTCS_F_SETFL) {
                m_fcntl_command = command;
                m_fcntl_value   = value;

                m_blocking = (value & HTCS_O_NONBLOCK) == 0;
            } else if (command == HTCS_F_GETFL) {
                res = m_fcntl_value;
            } else {
                last_error = HTCS_EINVAL;
                res        = -1;
            }

            return res;
        }

        s32 SetSocket(sf::SharedPointer<tma::ISocket> socket, s32 &last_error) {
            s32 res = 0;

            if (m_socket == nullptr && socket != nullptr) {
                /* Set our socket. */
                m_socket = socket;

                /* Bind, fcntl, and listen, since those may have been deferred. */
                if (m_do_bind) {
                    res = bind(m_socket, std::addressof(m_address), last_error);
                }

                if (res == 0 && m_fcntl_command != -1) {
                    res = fcntl(m_socket, m_fcntl_command, m_fcntl_value, last_error);
                }

                if (res == 0 && m_listen_backlog_count > 0) {
                    res = listen(m_socket, m_listen_backlog_count, last_error);
                }
            }

            return res;
        }
    };

    VirtualSocketCollection::VirtualSocketCollection()
        : m_socket_list(nullptr),
          m_list_count(0),
          m_list_size(0),
          m_next_id(1),
          m_mutex()
    {
        /* ... */
    }

    VirtualSocketCollection::~VirtualSocketCollection() {
        /* Clear ourselves. */
        this->Clear();

        /* Destroy all sockets in our list. */
        for (auto i = 0; i < m_list_size; ++i) {
            std::destroy_at(m_socket_list + i);
        }

        /* Clear the backing memory for our socket list. */
        std::memset(m_buffer, 0, sizeof(VirtualSocket) * m_list_size);
    }

    size_t VirtualSocketCollection::GetWorkingMemorySize(int num_sockets) {
        AMS_ASSERT(num_sockets < htcs::SocketCountMax);
        return num_sockets * sizeof(VirtualSocket);
    }

    void VirtualSocketCollection::Init(void *buffer, size_t buffer_size) {
        /* Set our buffer. */
        m_buffer      = buffer;
        m_buffer_size = buffer_size;

        /* Configure our list. */
        m_list_size = static_cast<s32>(m_buffer_size / sizeof(VirtualSocket));
        m_list_size = std::min(m_list_size, htcs::SocketCountMax);

        /* Initialize our list. */
        m_socket_list = static_cast<VirtualSocket *>(m_buffer);
        for (auto i = 0; i < m_list_size; ++i) {
            std::construct_at(m_socket_list + i);
        }
    }

    void VirtualSocketCollection::Clear() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Clear our list. */
        m_list_count = 0;
    }

    s32 VirtualSocketCollection::Socket(s32 &error_code) {
        /* Clear error code. */
        error_code = 0;

        /* Create the socket. */
        return this->CreateSocket(sf::SharedPointer<tma::ISocket>{nullptr}, error_code);
    }

    s32 VirtualSocketCollection::Close(s32 id, s32 &error_code) {
        /* Clear error code. */
        error_code = 0;

        /* Prepare to find the socket. */
        s32 res = 0;
        sf::SharedPointer<tma::ISocket> socket = nullptr;

        /* Find the socket. */
        s32 index;
        {
            std::scoped_lock lk(m_mutex);

            if (index = this->Find(id, std::addressof(error_code)); index >= 0) {
                /* Get the socket's object. */
                VirtualSocket *virt_socket = m_socket_list + index;
                socket = virt_socket->m_socket;

                /* Move the list. */
                for (/* ... */; index < m_list_count - 1; ++index) {
                    m_socket_list[index] = m_socket_list[index + 1];
                }

                /* Clear the now unused last list entry. */
                m_socket_list[index].Init();

                /* Decrement our list count. */
                --m_list_count;
            }
        }

        /* If we found the socket, close it. */
        if (socket != nullptr) {
            close(socket, error_code);

            /* Clear the error code. */
            res        = 0;
            error_code = 0;
        }

        return index >= 0 ? res : -1;
    }

    s32 VirtualSocketCollection::Bind(s32 id, const htcs::SockAddrHtcs *address, s32 &error_code) {
        /* Setup result/error code. */
        s32 res    = -1;
        error_code = 0;

        /* Get the socket. */
        sf::SharedPointer<tma::ISocket> socket = this->GetSocket(id, std::addressof(error_code));

        /* If we found the socket, bind. */
        if (socket != nullptr) {
            res = bind(socket, address, error_code);
        } else if (error_code != HTCS_EBADF) {
            /* Lock ourselves. */
            std::scoped_lock lk(m_mutex);

            /* Check if the socket is already bound. */
            if (const auto index = this->Find(id); index >= 0) {
                const auto exists = this->HasAddr(address);

                if (m_socket_list[index].m_do_bind) {
                    error_code = HTCS_EINVAL;
                    res        = -1;
                } else if (exists) {
                    error_code = HTCS_EADDRINUSE;
                    res        = -1;
                } else {
                    res = m_socket_list[index].Bind(address, error_code);
                }
            } else {
                error_code = HTCS_EBADF;
            }
        }

        return res;
    }
    s32 VirtualSocketCollection::Listen(s32 id, s32 backlog_count, s32 &error_code) {
        /* Setup result/error code. */
        s32 res    = -1;
        error_code = 0;

        /* Get the socket. */
        sf::SharedPointer<tma::ISocket> socket = this->GetSocket(id, std::addressof(error_code));

        /* If we found the socket, bind. */
        if (socket != nullptr) {
            res = listen(socket, backlog_count, error_code);
        } else if (error_code != HTCS_EBADF) {
            /* Lock ourselves. */
            std::scoped_lock lk(m_mutex);

            /* Try to listen on the virtual socket. */
            if (const auto index = this->Find(id); index >= 0) {
                res = m_socket_list[index].Listen(backlog_count, error_code);
            }
        }

        return res;
    }

    s32 VirtualSocketCollection::Accept(s32 id, htcs::SockAddrHtcs *address, s32 &error_code) {
        /* Setup result/error code. */
        s32 res    = -1;
        error_code = 0;

        /* Declare socket that we're creating. */
        sf::SharedPointer<tma::ISocket> new_socket = nullptr;

        /* Get the socket. */
        sf::SharedPointer<tma::ISocket> socket = this->GetSocket(id, std::addressof(error_code));

        /* If we found the socket, bind. */
        if (socket != nullptr) {
            if (error_code != HTCS_ENONE) {
                return -1;
            }

            new_socket = this->DoAccept(socket, id, address, error_code);
        } else if (error_code != HTCS_EBADF) {
            /* Fetch the socket. */
            socket = this->FetchSocket(id, error_code);

            /* Wait for the socket. */
            while (socket == nullptr) {
                /* Determine whether we should block/listen. */
                bool block_until_done = true;
                bool listened         = false;

                s32 index;
                {
                    std::scoped_lock lk(m_mutex);
                    if (index = this->Find(id, std::addressof(error_code)); index >= 0) {
                        block_until_done = m_socket_list[index].m_blocking;
                        listened         = m_socket_list[index].m_listen_backlog_count > 0;
                    }
                }

                /* Check that the socket exists. */
                if (index < 0) {
                    error_code = HTCS_EINTR;
                    return -1;
                }

                /* Check that the socket has been listened. */
                if (!listened) {
                    error_code = HTCS_EINVAL;
                    return -1;
                }

                /* Check that we should block. */
                if (!block_until_done) {
                    error_code = HTCS_EWOULDBLOCK;
                    return -1;
                }

                /* Wait before trying again. */
                os::SleepThread(TimeSpan::FromMilliSeconds(500));

                /* Fetch the potentially updated socket. */
                socket = this->FetchSocket(id, error_code);
            }

            /* Check that we haven't errored. */
            if (error_code != HTCS_ENONE) {
                return -1;
            }

            /* Do the accept. */
            new_socket = this->DoAccept(socket, id, address, error_code);
        }

        /* If we have a new socket, register it. */
        if (new_socket != 0) {
            res = this->CreateSocket(new_socket, error_code);
            if (res < 0) {
                s32 tmp_error_code;
                close(new_socket, tmp_error_code);
            }
        }

        return res;
    }

    s32 VirtualSocketCollection::Fcntl(s32 id, s32 command, s32 value, s32 &error_code) {
        /* Setup result/error code. */
        s32 res    = -1;
        error_code = 0;

        /* Get the socket. */
        sf::SharedPointer<tma::ISocket> socket = this->GetSocket(id, std::addressof(error_code));

        /* If we found the socket, bind. */
        if (socket != nullptr) {
            res = fcntl(socket, command, value, error_code);
        } else if (error_code != HTCS_EBADF) {
            /* Lock ourselves. */
            std::scoped_lock lk(m_mutex);

            /* Try to listen on the virtual socket. */
            if (const auto index = this->Find(id); index >= 0) {
                res = m_socket_list[index].Fcntl(command, value, error_code);
            }
        }

        return res;
    }

    s32 VirtualSocketCollection::Shutdown(s32 id, s32 how, s32 &error_code) {
        /* Setup result/error code. */
        s32 res    = -1;
        error_code = 0;

        /* Get the socket. */
        sf::SharedPointer<tma::ISocket> socket = this->GetSocket(id, std::addressof(error_code));

        /* If we found the socket, bind. */
        if (socket != nullptr) {
            res = shutdown(socket, how, error_code);
        } else if (error_code != HTCS_EBADF) {
            error_code = HTCS_ENOTCONN;
        }

        return res;
    }

    ssize_t VirtualSocketCollection::Recv(s32 id, void *buffer, size_t buffer_size, s32 flags, s32 &error_code) {
        /* Setup result/error code. */
        ssize_t res = -1;
        error_code  = 0;

        /* Fetch the socket. */
        sf::SharedPointer<tma::ISocket> socket = this->FetchSocket(id, error_code);

        /* If we found the socket, bind. */
        if (socket != nullptr) {
            if (error_code != HTCS_ENONE) {
                return -1;
            }
            res = recv(socket, buffer, buffer_size, flags, error_code);
        } else if (error_code != HTCS_EBADF) {
            error_code = HTCS_ENOTCONN;
        }

        return res;
    }

    ssize_t VirtualSocketCollection::Send(s32 id, const void *buffer, size_t buffer_size, s32 flags, s32 &error_code) {
        /* Setup result/error code. */
        ssize_t res = -1;
        error_code  = 0;

        /* Fetch the socket. */
        sf::SharedPointer<tma::ISocket> socket = this->FetchSocket(id, error_code);

        /* If we found the socket, bind. */
        if (socket != nullptr) {
            if (error_code != HTCS_ENONE) {
                return -1;
            }
            res = send(socket, buffer, buffer_size, flags, error_code);
        } else if (error_code != HTCS_EBADF) {
            error_code = HTCS_ENOTCONN;
        }

        return res;
    }

    s32 VirtualSocketCollection::Connect(s32 id, const htcs::SockAddrHtcs *address, s32 &error_code) {
        /* Setup result/error code. */
        s32 res    = -1;
        error_code = 0;

        /* Fetch the socket. */
        sf::SharedPointer<tma::ISocket> socket = this->FetchSocket(id, error_code);

        /* If we found the socket, bind. */
        if (socket != nullptr) {
            if (error_code != HTCS_ENONE) {
                return -1;
            }
            res = connect(socket, address, error_code);
        } else if (error_code != HTCS_EBADF) {
            error_code = HTCS_EADDRNOTAVAIL;
        }

        return res;
    }

    s32 VirtualSocketCollection::Select(htcs::FdSet *read, htcs::FdSet *write, htcs::FdSet *except, htcs::TimeVal *timeout, s32 &error_code) {
        /* Setup result/error code. */
        s32 res            = -1;
        s32 tmp_error_code = 0;

        /* Declare buffers. */
        s32 read_primitives[SocketCountMax];
        s32 write_primitives[SocketCountMax];
        s32 except_primitives[SocketCountMax];

        /* Get reads. */
        s32 num_read = this->GetSockets(read_primitives, read, tmp_error_code);
        if (tmp_error_code != HTCS_ENONE) {
            error_code = tmp_error_code;
            return res;
        }

        /* Get writes. */
        s32 num_write = this->GetSockets(write_primitives, write, tmp_error_code);
        if (tmp_error_code != HTCS_ENONE) {
            error_code = tmp_error_code;
            return res;
        }

        /* Get excepts. */
        s32 num_except = this->GetSockets(except_primitives, except, tmp_error_code);
        if (tmp_error_code != HTCS_ENONE) {
            error_code = tmp_error_code;
            return res;
        }

        /* Perform the select. */
        if (num_read + num_write + num_except > 0) {
            res = select(read_primitives, num_read, write_primitives, num_write, except_primitives, num_except, timeout, error_code);

            /* Set the socket primitives. */
            this->SetSockets(read, read_primitives, num_read);
            this->SetSockets(write, write_primitives, num_write);
            this->SetSockets(except, except_primitives, num_except);
        } else {
            error_code = HTCS_EINVAL;
        }

        return res;
    }

    s32 VirtualSocketCollection::CreateId() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Get a free id. */
        s32 res = 0;
        do {
            res = m_next_id++;
            if (m_next_id <= 0) {
                m_next_id = 1;
            }
        } while (this->Find(res) >= 0);

        return res;
    }

    s32 VirtualSocketCollection::Add(sf::SharedPointer<tma::ISocket> socket) {
        /* Check that the socket isn't null. */
        if (socket == nullptr) {
            return -1;
        }

        /* Create the socket. */
        s32 error_code;
        return this->CreateSocket(socket, error_code);
    }

    void VirtualSocketCollection::Insert(s32 id, sf::SharedPointer<tma::ISocket> socket) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Add the socket to the list. */
        if (m_list_count != 0) {
            /* Ensure the list remains in sorted order. */
            s32 index;
            for (index = m_list_count - 1; index >= 0; --index) {
                if (m_socket_list[index].m_id < id) {
                    break;
                }
                m_socket_list[index + 1] = m_socket_list[index];
            }

            /* Set the socket in the list. */
            m_socket_list[index + 1].m_id     = id;
            m_socket_list[index + 1].m_socket = socket;
        } else {
            /* Set the socket in the list. */
            m_socket_list[0].m_id     = id;
            m_socket_list[0].m_socket = socket;
        }

        /* Increment our count. */
        ++m_list_count;
    }

    void VirtualSocketCollection::SetSize(s32 size) {
        AMS_UNUSED(size);
    }

    s32 VirtualSocketCollection::Find(s32 id, s32 *error_code) {
        /* Perform a binary search to find the socket. */
        if (m_list_count > 0) {
            s32 left  = 0;
            s32 right = m_list_count - 1;
            while (left <= right) {
                const s32 mid = (left + right) / 2;
                if (m_socket_list[mid].m_id == id) {
                    return mid;
                } else if (m_socket_list[mid].m_id > id) {
                    right = mid - 1;
                } else /* if (m_socket_list[mid].m_id < id) */ {
                    left  = mid + 1;
                }
            }
        }

        /* We failed to find the socket. */
        if (error_code != nullptr) {
            *error_code = HTCS_EBADF;
        }

        return InvalidSocket;
    }

    s32 VirtualSocketCollection::FindByPrimitive(s32 primitive) {
        /* Find a socket with the desired primitive. */
        for (auto i = 0; i < m_list_size; ++i) {
            if (m_socket_list[i].m_primitive == primitive) {
                return i;
            }
        }

        return InvalidPrimitive;
    }

    bool VirtualSocketCollection::HasAddr(const htcs::SockAddrHtcs *address) {
        /* Try to find a matching socket. */
        for (auto i = 0; i < m_list_count; ++i) {
            if (m_socket_list[i].m_address.family == address->family &&
                std::strcmp(m_socket_list[i].m_address.peer_name.name, address->peer_name.name) == 0 &&
                std::strcmp(m_socket_list[i].m_address.port_name.name, address->port_name.name) == 0)
            {
                return true;
            }
        }
        return false;
    }

    sf::SharedPointer<tma::ISocket> VirtualSocketCollection::GetSocket(s32 id, s32 *error_code) {
        sf::SharedPointer<tma::ISocket> res = nullptr;

        /* Get the socket. */
        {
            std::scoped_lock lk(m_mutex);

            if (const auto index = this->Find(id, error_code); index >= 0) {
                res = m_socket_list[index].m_socket;
            }
        }

        return res;
    }

    sf::SharedPointer<tma::ISocket> VirtualSocketCollection::FetchSocket(s32 id, s32 &error_code) {
        /* Clear the error code. */
        error_code = 0;

        /* Get the socket. */
        auto socket = this->GetSocket(id, std::addressof(error_code));
        if (socket == nullptr && error_code == HTCS_ENONE) {
            socket = this->RealizeSocket(id);
        }

        return socket;
    }

    sf::SharedPointer<tma::ISocket> VirtualSocketCollection::RealizeSocket(s32 id) {
        /* Clear the error code. */
        s32 error_code = 0;

        /* Get socket. */
        sf::SharedPointer<tma::ISocket> res = socket(error_code);
        if (res != nullptr) {
            /* Assign the new socket. */
            s32 index;
            {
                std::scoped_lock lk(m_mutex);

                index = this->Find(id, std::addressof(error_code));
                if (index >= 0) {
                    m_socket_list[index].SetSocket(res, error_code);
                }
            }

            /* If the socket was deleted, close it. */
            if (index < 0) {
                s32 temp_error = 0;
                close(res, temp_error);
                res = nullptr;
            }
        }

        return res;
    }

    sf::SharedPointer<tma::ISocket> VirtualSocketCollection::DoAccept(sf::SharedPointer<tma::ISocket> socket, s32 id, htcs::SockAddrHtcs *address, s32 &error_code) {
        /* Clear the error code. */
        error_code = 0;

        /* Try to accept. */
        sf::SharedPointer<tma::ISocket> new_socket = accept(socket, address, error_code);
        if (error_code == HTCS_ENETDOWN) {
            new_socket = accept(socket, address, error_code);

            std::scoped_lock lk(m_mutex);
            if (const auto index = this->Find(id, std::addressof(error_code)); index >= 0) {
                m_socket_list[index].m_socket = nullptr;
            }
        }

        return new_socket;
    }

    s32 VirtualSocketCollection::GetSockets(s32 * const out_primitives, htcs::FdSet *set, s32 &error_code) {
        /* Clear the error code. */
        error_code = 0;
        s32 count  = 0;

        /* Walk the fdset. */
        if (set != nullptr) {
            for (auto i = 0; i < FdSetSize; ++i) {
                /* If the set no longer has fds, we're done. */
                if (set->fds[i] == 0) {
                    break;
                }

                /* Find the fd's primitive. */
                s32 primitive = InvalidPrimitive;
                s32 index;
                {
                    std::scoped_lock lk(m_mutex);
                    if (index = this->Find(set->fds[i], std::addressof(error_code)); index >= 0) {
                        /* Get the primitive, if necessary. */
                        if (m_socket_list[index].m_primitive == InvalidPrimitive && m_socket_list[index].m_socket != nullptr) {
                            m_socket_list[index].m_socket->GetPrimitive(std::addressof(m_socket_list[index].m_primitive));
                        }

                        primitive = m_socket_list[index].m_primitive;
                    }
                }

                /* Check that an error didn't occur. */
                if (error_code != HTCS_ENONE) {
                    return 0;
                }

                /* If the primitive is invalid, try to realize the socket. */
                if (primitive == InvalidPrimitive) {
                    if (this->RealizeSocket(set->fds[i]) != nullptr) {
                        std::scoped_lock lk(m_mutex);

                        /* Get the primitive. */
                        if (index = this->Find(set->fds[i], std::addressof(error_code)); index >= 0) {
                            m_socket_list[index].m_socket->GetPrimitive(std::addressof(m_socket_list[index].m_primitive));

                            primitive = m_socket_list[index].m_primitive;
                        }
                    }

                    /* Check that an error didn't occur. */
                    if (error_code != HTCS_ENONE) {
                        return 0;
                    }
                }

                /* Set the output primitive. */
                if (primitive != InvalidPrimitive) {
                    out_primitives[count++] = primitive;
                }
            }
        }

        return count;
    }

    void VirtualSocketCollection::SetSockets(htcs::FdSet *set, s32 * const primitives, s32 count) {
        if (set != nullptr) {
            /* Clear the set. */
            FdSetZero(set);

            /* Copy the fds. */
            for (auto i = 0; i < count; ++i) {
                std::scoped_lock lk(m_mutex);

                if (const auto index = this->FindByPrimitive(primitives[i]); index >= 0) {
                    set->fds[i] = m_socket_list[index].m_id;
                }
            }
        }
    }

    s32 VirtualSocketCollection::CreateSocket(sf::SharedPointer<tma::ISocket> socket, s32 &error_code) {
        /* Clear the error code. */
        error_code = 0;
        s32 id     = InvalidSocket;

        /* Check that we can add to the list. */
        if (m_list_count < m_list_size) {
            /* Create a new id. */
            id = this->CreateId();

            /* Insert the socket into the list. */
            this->Insert(id, socket);
        } else {
            if (socket != nullptr) {
                s32 tmp_error_code;
                close(socket, tmp_error_code);
            }

            error_code = HTCS_EMFILE;
        }

        return id;
    }

}
