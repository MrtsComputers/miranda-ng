/*

Facebook plugin for Miranda Instant Messenger
_____________________________________________

Copyright � 2009-11 Michal Zelinka, 2011-13 Robert P�sel

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

struct facebook_user
{
	HANDLE handle;

	std::string user_id;
	std::string real_name;

	unsigned int status_id;
	unsigned int gender;
	DWORD last_active;

	std::string image_url;

	bool deleted;

	facebook_user()
	{
		this->handle = NULL;
		this->user_id = this->real_name = this->image_url = "";
		this->status_id = ID_STATUS_OFFLINE;
		this->gender = this->last_active = 0;
		this->deleted = false;
	}

	facebook_user(facebook_user* fu)
	{
		this->handle = fu->handle;
		this->image_url = fu->image_url;
		this->real_name = fu->real_name;
		this->status_id = fu->status_id;
		this->user_id = fu->user_id;
		this->gender = fu->gender;
		this->last_active = fu->last_active;
		this->deleted = fu->deleted;
	}
};

struct facebook_chatroom
{
	HANDLE handle;

	std::string chat_name;
	std::map<std::string, std::string> participants;

	std::string message_readers;

	DWORD last_active;
};

struct facebook_message
{
	std::string user_id;
	std::string message_text;
	std::string sender_name;
	std::string message_id;
	DWORD time;
	bool isIncoming;
	bool isUnread;
	bool isChat;

	facebook_message()
	{
		this->user_id = this->message_text = this->sender_name = this->message_id = "";
		this->time = 0;
		this->isUnread = this->isIncoming = true;
		this->isChat = false;
	}

	facebook_message(const facebook_message& msg)
	{
		this->user_id = msg.user_id;
		this->message_text = msg.message_text;
		this->sender_name = msg.sender_name;
		this->message_id = msg.message_id;
		this->time = msg.time;
		this->isIncoming = msg.isIncoming;
		this->isUnread = msg.isUnread;
		this->isChat = msg.isChat;
	}
};

struct facebook_notification
{
	std::string user_id;
	std::string text;
	std::string link;
	std::string id;

	facebook_notification()
	{
		this->user_id = this->text = this->link = this->id = "";
	}
};

struct facebook_newsfeed
{
	std::string user_id;
	std::string title;
	std::string text;
	std::string link;

	facebook_newsfeed()
	{
		this->user_id = this->title = this->text = this->link = "";
	}
};

struct send_chat
{
	send_chat(const std::string &chat_id,const std::string &msg) : chat_id(chat_id), msg(msg) {}
	std::string chat_id;
	std::string msg;
};

struct send_direct
{
	send_direct(HANDLE hContact,const std::string &msg, HANDLE msgid) : hContact(hContact), msg(msg), msgid(msgid) {}
	HANDLE hContact;
	std::string msg;
	HANDLE msgid;
};

struct send_typing
{
	send_typing(HANDLE hContact,const int status) : hContact(hContact), status(status) {}
	HANDLE hContact;
	int status;
};

struct popup_data
{
	popup_data(FacebookProto *proto) : proto(proto) {}
	popup_data(FacebookProto *proto, std::string url) : proto(proto), url(url) {}
	FacebookProto *proto;
	std::string url;
	std::string notification_id;
};

struct status_data
{
	status_data() {
		this->user_id = this->text = this->url = this->place = this->privacy = "";
		this->isPage = false;
	}
	std::string user_id;
	std::string text;
	std::string url;	
	std::string place;	
	std::string privacy;
	bool isPage;
	std::vector<facebook_user*> users;
};

struct wall_data
{
	wall_data() {
		this->user_id = "";
		this->title = NULL;
		this->isPage = false;
	}
	wall_data(std::string user_id, TCHAR *title, bool isPage = false) : user_id(user_id), title(title), isPage(isPage) {}
	std::string user_id;
	TCHAR *title;
	bool isPage;
};

struct post_status_data {
	post_status_data(FacebookProto *proto) : proto(proto) {}
	post_status_data(FacebookProto *proto, wall_data *wall) : proto(proto) {
		this->walls.push_back(wall);
	}
	FacebookProto *proto;
	std::vector<wall_data*> walls;
};
