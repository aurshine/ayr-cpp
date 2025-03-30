#ifndef AYR_NET_SELECT_HPP
#define AYR_NET_SELECT_HPP

#include "Socket.hpp"
#include "../base/View.hpp"

namespace ayr
{
	struct FdSet : public Object<FdSet>
	{
		fd_set fds;

		int max_fd;

		FdSet() : fds(), max_fd(0) {}

		void add(const Socket& socket) { FD_SET(socket.fd(), &fds); max_fd = std::max(max_fd, socket.fd()); }

		void pop(const Socket& socket) { FD_CLR(socket.fd(), &fds); }

		bool contains(const Socket& socket) const { return FD_ISSET(socket.fd(), &fds); }

		void clear() { FD_ZERO(&fds); }
	};

	struct SelectEvent : public Object<SelectEvent>
	{
		constexpr static int READ = 0x01;

		constexpr static int WRITE = 0x02;

		constexpr static int EXCEPT = 0x04;

		int events;

		View data;
	};

	class Select : public Object<Select>
	{
		using self = Select;

		using super = Object<Select>;

		FdSet* read_set, * write_set, * except_set;

		Array<View> datas;
	public:
		Select() :
			read_set(nullptr),
			write_set(nullptr),
			except_set(nullptr),
			datas(FD_SETSIZE)
		{}

		~Select()
		{
			ayr_desloc(read_set);
			ayr_desloc(write_set);
			ayr_desloc(except_set);
		}

		bool contains(const Socket& socket) const
		{
			if (read_set && read_set->contains(socket))
				return true;
			if (write_set && write_set->contains(socket))
				return true;
			if (except_set && except_set->contains(socket))
				return true;
			return false;
		}

		void set(const Socket& socket, bool read, bool write, bool except = false)
		{
			set_impl(socket, read_set, read);
			set_impl(socket, write_set, write);
			set_impl(socket, except_set, except);
		}

		void set(const Socket& socket, const View& data, bool read, bool write, bool except = false)
		{
			set(socket, read, write, except);
			datas[socket.fd()] = data;
		}

		void del(const Socket& socket)
		{
			if (read_set) read_set->pop(socket);
			if (write_set) write_set->pop(socket);
			if (except_set) except_set->pop(socket);
		}

		void clear()
		{
			if (read_set) read_set->clear();
			if (write_set) write_set->clear();
			if (except_set) except_set->clear();
		}

		Array<SelectEvent> wait(int time_ms)
		{
			timeval timeout{ time_ms / 1000, (time_ms % 1000) * 1000 };
			timeout.tv_sec = time_ms / 1000;
			fd_set tmp_read_set, tmp_write_set, tmp_except_set;
			if (read_set) tmp_read_set = read_set->fds;
			if (write_set) tmp_write_set = write_set->fds;
			if (except_set) tmp_except_set = except_set->fds;

			int n = select(FD_SETSIZE, &tmp_read_set, &tmp_write_set, &tmp_except_set, &timeout);

			Array<SelectEvent> events(n);
			for (int i = 0, j = 0; j < FD_SETSIZE && i < n; ++j)
			{
				bool flag = false;
				if (read_set->contains(j))
				{
					flag = true;
					events[i].events |= SelectEvent::READ;
				}

				if (write_set->contains(j))
				{
					flag = true;
					events[i].events |= SelectEvent::WRITE;
				}

				if (except_set->contains(j))
				{
					flag = true;
					events[i].events |= SelectEvent::EXCEPT;
				}

				if (flag)
				{
					events[i].data = datas[j];
					++i;
				}
			}

			return events;
		}
	private:
		void set_impl(const Socket& socket, FdSet*& fds, bool st)
		{
			if (st)
			{
				if (!fds) fds = ayr_make<FdSet>();
				fds->add(socket);
			}
			else
			{
				if (fds) fds->pop(socket);
			}
		}

		void set_impl(const Socket& socket, const View& view, FdSet*& fds, bool st)
		{
			if (st)
			{
				if (!fds) fds = ayr_make<FdSet>();
				fds->add(socket);
				datas[socket.fd()] = view;
			}
			else
			{
				if (fds) fds->pop(socket);
			}
		}
	};
}

#endif // AYR_NET_SELECT_HPP