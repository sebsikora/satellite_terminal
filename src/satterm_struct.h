// -----------------------------------------------------------------------------------------------------
// satellite_terminal - Easily spawn and communicate bidirectionally with client processes in separate
//                      terminal emulator instances.
// -----------------------------------------------------------------------------------------------------
// seb.nf.sikora@protonmail.com
//
// Copyright © 2021 Dr Seb N.F. Sikora.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.
// -----------------------------------------------------------------------------------------------------

struct fifo {
	std::string identifier;
	bool created;
	bool opened;
	int descriptor;
	
	fifo& operator=(fifo const& rhs) {
		identifier = rhs.identifier;
		created = rhs.created;
		opened = rhs.opened;
		descriptor = rhs.descriptor;
		return *this;
	}
};

struct fifo_pair {
	fifo in;
	fifo out;
	
	fifo_pair& operator=(fifo_pair const& rhs) {
		in = rhs.in;
		out = rhs.out;
		return *this;
	}
};

struct error_descriptor {
	int err_no;
	std::string err_detail;
	
	error_descriptor& operator=(error_descriptor const& rhs) {
		err_no = rhs.err_no;
		err_detail = rhs.err_detail;
		return *this;
	}
	bool operator==(error_descriptor const& rhs) const {
		return ((this->err_no == rhs.err_no) && (this->err_detail == rhs.err_detail));
	}
	bool operator!=(error_descriptor const& rhs) const {
		return ((this->err_no != rhs.err_no) || (this->err_detail != rhs.err_detail));
	}
};
