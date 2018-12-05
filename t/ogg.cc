#include <opustags.h>
#include "tap.h"

#include <string.h>

static void check_ref_ogg()
{
	ot::file input = fopen("gobble.opus", "r");
	if (input == nullptr)
		throw failure("could not open gobble.opus");

	ot::ogg_reader reader(input.get());

	ot::status rc = reader.read_page();
	if (rc != ot::st::ok)
		throw failure("could not read the first page");
	rc = reader.read_header_packet([](ogg_packet& p) {
		if (p.bytes != 19)
			throw failure("unexpected length for the first packet");
		return ot::st::ok;
	});
	if (rc != ot::st::ok)
		throw failure("could not read the first packet");

	rc = reader.read_page();
	if (rc != ot::st::ok)
		throw failure("could not read the second page");
	rc = reader.read_header_packet([](ogg_packet& p) {
		if (p.bytes != 62)
			throw failure("unexpected length for the second packet");
		return ot::st::ok;
	});
	if (rc != ot::st::ok)
		throw failure("could not read the second packet");

	while (!ogg_page_eos(&reader.page)) {
		rc = reader.read_page();
		if (rc != ot::st::ok)
			throw failure("failure reading a page");
	}
	rc = reader.read_page();
	if (rc != ot::st::end_of_stream)
		throw failure("did not correctly detect the end of stream");
}

static ogg_packet make_packet(const char* contents)
{
	ogg_packet op {};
	op.bytes = strlen(contents);
	op.packet = (unsigned char*) contents;
	return op;
}

static bool same_packet(const ogg_packet& lhs, const ogg_packet& rhs)
{
	return lhs.bytes == rhs.bytes && memcmp(lhs.packet, rhs.packet, lhs.bytes) == 0;
}

/**
 * Build an in-memory Ogg stream using ogg_writer, and then read it with ogg_reader.
 */
static void check_memory_ogg()
{
	ogg_packet first_packet  = make_packet("First");
	ogg_packet second_packet = make_packet("Second");
	std::vector<unsigned char> my_ogg(128);
	size_t my_ogg_size;
	ot::status rc;

	{
		ot::file output = fmemopen(my_ogg.data(), my_ogg.size(), "w");
		if (output == nullptr)
			throw failure("could not open the output stream");
		ot::ogg_writer writer(output.get());
		writer.write_header_packet(1234, 0, first_packet);
		if (rc != ot::st::ok)
			throw failure("could not write the first packet");
		writer.write_header_packet(1234, 1, second_packet);
		if (rc != ot::st::ok)
			throw failure("could not write the second packet");
		my_ogg_size = ftell(output.get());
		if (my_ogg_size != 67)
			throw failure("unexpected output size");
	}

	{
		ot::file input = fmemopen(my_ogg.data(), my_ogg_size, "r");
		if (input == nullptr)
			throw failure("could not open the input stream");
		ot::ogg_reader reader(input.get());
		rc = reader.read_page();
		if (rc != ot::st::ok)
			throw failure("could not read the first page");
		rc = reader.read_header_packet([&first_packet](ogg_packet &p) {
			if (!same_packet(p, first_packet))
				throw failure("unexpected content in the first packet");
			return ot::st::ok;
		});
		if (rc != ot::st::ok)
			throw failure("could not read the first packet");
		rc = reader.read_page();
		if (rc != ot::st::ok)
			throw failure("could not read the second page");
		rc = reader.read_header_packet([&second_packet](ogg_packet &p) {
			if (!same_packet(p, second_packet))
				throw failure("unexpected content in the second packet");
			return ot::st::ok;
		});
		if (rc != ot::st::ok)
			throw failure("could not read the second packet");
		rc = reader.read_page();
		if (rc != ot::st::end_of_stream)
			throw failure("unexpected third page");
	}
}

int main(int argc, char **argv)
{
	std::cout << "1..2\n";
	run(check_ref_ogg, "check a reference ogg stream");
	run(check_memory_ogg, "build and check a fresh stream");
	return 0;
}
