std::vector<uint8_t> bits;
image.input( L"digits.png" );
for( int i = 0; i < image.h(); ++i )
{
	for( int j = 0; j < image.w(); ++j )
	{
		auto& p = *image( j, i );
		if( p.r == 0 && p.g == 0 && p.b == 255 )
			continue;
		bits.push_back( p.r == 255 && p.g == 255 && p.b == 0 );
	}
}

auto remainder = bits.size() % 32;
if( remainder > 0 )
{
	remainder = 32 - remainder;
	while( remainder > 0 )
	{
		bits.push_back( 0 );
		--remainder;
	}
}

int i = 0;
uint32_t byte = 0;
std::vector<uint32_t> bytes;
for( auto bit : bits )
{
	byte *= 2;
	byte += bit;
	++i;

	if( i == 32 )
	{
		bytes.push_back( byte );
		byte = 0;
		i = 0;
	}
}

for( auto b : bytes )
	text << b << ", ";

std::vector<ImageData> ico;
ico.emplace_back().input( L"graph.png" );
ImageData::writeICO( L"/graph.ico", ico );
