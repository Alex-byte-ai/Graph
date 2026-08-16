#include "Graph.h"

#include "Exception.h"
#include "Line.h"

#include <algorithm>
#include <fstream>
#include <math.h>
#include <map>

static void drawLine( uint32_t *pixels, int width, int height, int x0, int y0, int x1, int y1 )
{
    auto line = [&]( auto call )
    {
        DrawLine l( x0, y0, x1, y1 );
        do
        {
            auto x = l.x();
            auto y = l.y();
            if( 0 <= x && x < width && 0 <= y && y < height )
                call( x, y );

        }
        while( l.nextPixel() );
    };

    int n = 0;
    line( [&]( int, int )
    {
        ++n;
    } );

    if( n <= 1 )
        return;
    --n;

    int k = 0;
    line( [&]( int x, int y )
    {
        pixels[y * width + x] = 0xff000000 + ( ( 255 * k / n ) << 16 );
        ++k;
    } );
}

static bool rectanglePoint( int x, int y, int w, int h, int index, int& cx, int& cy )
{
    switch( index )
    {
    case 0:
        cx = x;
        cy = y;
        return true;
    case 1:
        cx = x + w;
        cy = y;
        return true;
    case 2:
        cx = x + w;
        cy = y + h;
        return true;
    case 3:
        cx = x;
        cy = y + h;
        return true;
    default:
        break;
    }
    return false;
}

static bool vectorConner( int sx, int sy, int fx, int fy, int index )
{
    int dx = fx - sx;
    int dy = fy - sy;

    if( index == 0 )
        return dx > 0 && dy > 0;
    if( index == 1 )
        return dx < 0 && dy > 0;
    if( index == 2 )
        return dx < 0 && dy < 0;
    if( index == 3 )
        return dx > 0 && dy < 0;
    return false;
}

static void drawArrow( uint32_t *pixels, int width, int height, int x0, int y0, int x1, int y1, int size )
{
    drawLine( pixels, width, height, x0, y0, x1, y1 );

    int i0 = y1 - size, i1 = y1 + size - 1;
    int j0 = x1 - size, j1 = x1 + size - 1;

    for( int i = i0; i <= i1; ++i )
    {
        int delta = i == i0 || i == i1;
        for( int j = j0 + delta; j <= j1 - delta; ++j )
        {
            if( 0 <= j && j < width && 0 <= i && i < height )
                pixels[i * width + j] = 0xffff0000;
        }
    }
}

Graph::Node::Node( Graph *graph, size_t index ): root( graph ), id( index )
{
    color = white;
    x = 0;
    y = 0;
}

bool Graph::Node::connect( Node *o )
{
    if( !o )
        return false;

    auto i = std::find( outputs.begin(), outputs.end(), o );
    if( i == outputs.end() )
    {
        outputs.push_back( o );
        o->inputs.push_back( this );
        return true;
    }

    outputs.erase( i );
    i = std::find( o->inputs.begin(), o->inputs.end(), this );
    if( i != o->inputs.end() )
        o->inputs.erase( i );

    return false;
}

Graph::Node* Graph::Node::input( size_t i )
{
    if( i < inputs.size() )
        return inputs[i];
    return nullptr;
}

Graph::Node* Graph::Node::output( size_t i )
{
    if( i < outputs.size() )
        return outputs[i];
    return nullptr;
}

size_t Graph::Node::inputSize() const
{
    return inputs.size();
}

size_t Graph::Node::outputSize() const
{
    return outputs.size();
}

size_t Graph::Node::index() const
{
    return id;
}

float Graph::Node::widthFix( float sx, bool levelBelow, size_t count ) const
{
    size_t n = outputs.size();
    if( count > n )
        count = n;

    float result = 0.0;
    for( size_t i = 0; i < count; ++i )
        result += outputs[i]->widthFix( sx ) + ( i + 1 < n ? sx : 0.0 );

    if( !levelBelow )
    {
        if( result < w )
            result = w;
    }

    return result;
}

bool Graph::Node::click( float x0, float y0 ) const
{
    return x <= x0 && x0 < x + w && y <= y0 && y0 < y + h;
}

void Graph::Node::draw( uint32_t *pixels, int width, int height )
{
    constexpr uint32_t characterData[53] = {0, 529294104, 4234217538, 284959228, 570393663, 142547143, 3775430408, 2082602977, 264650744, 1108378687, 2357139711, 4052730399, 2438069347, 524697228, 2096644624, 2214402246, 838328591, 3256876065, 4165078668, 557365809, 2414024848, 2216757312, 2216758365, 415748492, 1627931152, 4235679942, 831288153, 3324939363, 414962063, 2701670961, 2355294022, 1050977016, 1103171518, 1108378660, 1662567982, 2355304616, 2441896797, 3325120789, 415640097, 139428932, 1144913920, 507904, 939524096, 1192230912, 1145323520, 2181570688, 2, 0, 1074790401, 131072, 570966544, 2149818897, 268697600};

    if( !image.empty() )
    {
        for( int i = y; i < y + h; ++i )
        {
            for( int j = x; j < x + w; ++j )
            {
                if( 0 <= j && j < width && 0 <= i && i < height )
                {
                    auto& input = image[( i - y ) * w + ( j - x )];
                    auto inA = ( input >> 24 ) & 0xff;
                    auto inR = ( input >> 16 ) & 0xff;
                    auto inG = ( input >> 8 ) & 0xff;
                    auto inB = input & 0xff;

                    auto& output = pixels[i * width + j];
                    auto outR = ( output >> 16 ) & 0xff;
                    auto outG = ( output >> 8 ) & 0xff;
                    auto outB = output & 0xff;

                    auto revA = 255 - inA;
                    outR = ( revA * outR + inA * inR ) / 255;
                    outG = ( revA * outG + inA * inG ) / 255;
                    outB = ( revA * outB + inA * inB ) / 255;
                    output = ( 0xff << 24 ) | ( outR << 16 ) | ( outG << 8 ) | outB;
                }
            }
        }
        return;
    }

    w = text.size() * 6 + 2;
    h = 9;

    for( int i = y; i < y + h; ++i )
    {
        for( int j = x; j < x + w; ++j )
        {
            if( 0 <= j && j < width && 0 <= i && i < height )
                pixels[i * width + j] = 0xff000000;
        }
    }

    int k = 0;
    for( auto& c : text )
    {
        unsigned index = 47;
        if( c == L' ' )
        {
            index = 0;
        }
        else if( L'0' <= c && c <= L'9' )
        {
            index = c - L'0' + 1;
        }
        else if( L'a' <= c && c <= L'z' )
        {
            index = c - L'a' + 11;
        }
        else if( L'A' <= c && c <= L'Z' )
        {
            index = c - L'A' + 11;
        }
        else if( c == L'_' )
        {
            index = 37;
        }
        else if( c == L'-' )
        {
            index = 38;
        }
        else if( c == L'+' )
        {
            index = 39;
        }
        else if( c == L'/' )
        {
            index = 40;
        }
        else if( c == L'\\' )
        {
            index = 41;
        }
        else if( c == L'.' )
        {
            index = 42;
        }
        else if( c == L',' )
        {
            index = 43;
        }
        else if( c == L':' )
        {
            index = 44;
        }
        else if( c == L';' )
        {
            index = 45;
        }
        else if( c == L'!' )
        {
            index = 46;
        }

        for( int i = 0; i < 7; ++i )
        {
            for( int j = 0; j < 5; ++j )
            {
                auto bit = index * 35 + 5 * i + j;
                auto count = bit / 32;
                auto pixel = bit % 32;

                auto px = x + 6 * k + j + 1;
                auto py = y + i + 1;
                if( 0 <= px && px < width && 0 <= py && py < height )
                    pixels[py * width + px] = ( characterData[count] >> ( 31 - pixel ) ) % 2 ? 0xffffffff : 0xff000000;
            }
        }

        ++k;
    }
}

std::shared_ptr<Graph::Node> Graph::Node::detach()
{
    for( auto& input : inputs )
        input->remove( this );

    for( auto& output : outputs )
        output->remove( this );

    auto p = root->nodes.begin() + id;
    auto result = std::move( *p );
    root->nodes.erase( p );

    size_t i = 0;
    for( auto& node : root->nodes )
        node->id = i++;

    return result;
}

void Graph::Node::remove( Node* o )
{
    size_t i, n;

    n = inputs.size();
    for( i = 0; i < n; ++i )
    {
        if( inputs[i] == o )
        {
            inputs.erase( inputs.begin() + i );
            return;
        }
    }

    n = outputs.size();
    for( i = 0; i < n; ++i )
    {
        if( outputs[i] == o )
        {
            outputs.erase( outputs.begin() + i );
            return;
        }
    }
}

bool Graph::Node::cycle()
{
    color = gray;

    for( auto& v : outputs )
    {
        if( v->color == white )
        {
            if( v->cycle() )
                return true;
        }
        else if( v->color == gray )
        {
            return true;
        }
    }

    color = black;
    return false;
}

void Graph::Node::sortAsTree( float sx, float sy )
{
    auto calculate = [sx]( Node * node )
    {
        return ( node->widthFix( sx ) - node->w ) * 0.5;
    };

    auto shift = -calculate( this );

    float dy = sy;
    if( h > dy )
        dy = h;

    size_t i = 0;
    for( auto& o : outputs )
    {
        o->x = x + shift + widthFix( sx, true, i ) + calculate( o );
        o->y = y + dy;
        o->sortAsTree( sx, sy );
        ++i;
    }
}

Graph::Graph()
{
    clear();
}

Graph::Graph( size_t n ): Graph()
{
    add( n );
}

Graph::~Graph()
{}

void Graph::clear()
{
    version = Version::v0;
    nodes.clear();
}

void Graph::add( size_t n )
{
    while( n > 0 )
    {
        nodes.push_back( std::make_shared<Node>( this, nodes.size() ) );
        --n;
    }
}

size_t Graph::length() const
{
    return nodes.size();
}

Graph::Node* Graph::get( size_t index )
{
    if( index < nodes.size() )
        return nodes[index].get();
    return nullptr;
}

void Graph::getCppStructure( const std::filesystem::path& path )
{
    std::map<std::filesystem::path, size_t> files;
    for( const auto& entry : std::filesystem::recursive_directory_iterator( path.parent_path() ) )
    {
        if( entry.is_regular_file() )
            files.emplace( entry.path(), 0 );
    }

    size_t i = 0;
    for( auto& [entry, id] : files )
        id = i++;

    clear();
    add( files.size() );

    for( auto& [entry, id] : files )
    {
        auto node = get( id );
        node->text = std::filesystem::relative( entry, path.parent_path() );

        std::wifstream file( entry );
        makeException( file.is_open() );

        std::wstring line;
        std::vector<std::wstring> lines;
        while( std::getline( file, line ) )
        {
            if( line.find( L"#include" ) != std::wstring::npos )
                lines.push_back( line );
        }

        for( auto& [oentry, oid] : files )
        {
            auto onode = get( oid );
            for( auto& l : lines )
            {
                if( l.find( oentry.filename().wstring() ) != std::wstring::npos )
                    node->connect( onode );
            }
        }
    }
}

void Graph::save( const std::filesystem::path & path ) const
{
    std::ofstream f( path, std::ios::binary );
    if( !f )
        return;

    f.write( "graph", 5 );
    f.write( ( char* )&version, sizeof( version ) );

    auto n = nodes.size();
    f.write( ( char* )&n, sizeof( n ) );

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;

        auto m = v.outputSize();
        f.write( ( char* )&m, sizeof( m ) );
        for( size_t i = 0; i < m; ++i )
        {
            auto id = v.output( i )->index();
            f.write( ( char* )&id, sizeof( id ) );
        }
    }

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;
        f.write( ( char* )v.text.c_str(), ( v.text.length() + 1 ) * sizeof( wchar_t ) );
    }

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;
        f.write( ( char* )&v.x, sizeof( v.x ) );
        f.write( ( char* )&v.y, sizeof( v.y ) );
    }

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;
        if( v.image.size() > 0 && v.w > 0 && v.h > 0 )
        {
            f.write( ( char* )&v.w, sizeof( v.w ) );
            f.write( ( char* )&v.h, sizeof( v.h ) );
            f.write( ( char* )v.image.data(), v.image.size() * sizeof( v.image[0] ) );
        }
        else
        {
            auto zero = v.w;
            zero = 0;
            f.write( ( char* )&zero, sizeof( zero ) );
        }
    }
}

bool Graph::load( const std::filesystem::path & path )
{
    clear();

    std::ifstream f( path, std::ios::binary );
    if( !f )
        return false;

    char ext[6] = {};
    if( !f.read( ext, 5 ) )
        return false;

    if( std::string( ext ) != "graph" )
        return false;

    if( !f.read( ( char* )&version, sizeof( version ) ) )
        return false;

    if( version != Version::v0 )
        return false;

    size_t n = 0;
    if( !f.read( ( char* )&n, sizeof( n ) ) )
        return false;

    add( n );

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;

        size_t m = 0;
        if( !f.read( ( char* )&m, sizeof( m ) ) )
            return false;

        for( size_t j = 0; j < m; ++j )
        {
            size_t id = 0;
            if( !f.read( ( char* )&id, sizeof( id ) ) )
                return false;

            if( !v.connect( get( id ) ) )
                return false;
        }
    }

    auto get = [&]( wchar_t& symbol )
    {
        if( !f.read( ( char* )&symbol, sizeof( symbol ) ) )
        {
            symbol = L'\0';
            return false;
        }
        return symbol != L'\0';
    };

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;
        wchar_t symbol = L'\0';
        while( get( symbol ) )
            v.text += symbol;
    }

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;

        if( !f.read( ( char* )&v.x, sizeof( v.x ) ) )
            v.x = 0;

        if( !f.read( ( char* )&v.y, sizeof( v.y ) ) )
            v.y = 0;
    }

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;

        if( !f.read( ( char* )&v.w, sizeof( v.w ) ) || v.w < 0 )
            v.w = 0;

        if( v.w == 0 )
            continue;

        if( !f.read( ( char* )&v.h, sizeof( v.h ) ) || v.h <= 0 )
        {
            v.h = 0;
            continue;
        }

        v.image.resize( v.w * v.h, 0 );
        f.read( ( char* )v.image.data(), v.image.size() * sizeof( v.image[0] ) );
    }

    return true;
}

void Graph::boundingBox( int& x, int& y, int& w, int& h ) const
{
    int xf, yf, xw, yh;

    x = y = std::numeric_limits<int>::max();
    xf = yf = std::numeric_limits<int>::lowest();

    for( auto& pointer : nodes )
    {
        auto& v = *pointer;

        if( v.x < x )
            x = v.x;

        if( v.y < y )
            y = v.y;

        xw = v.x + v.w;
        yh = v.y + v.h;

        if( xw > xf )
            xf = xw;

        if( yh > yf )
            yf = yh;
    }

    if( x < xf && y < yf )
    {
        w = xf - x;
        h = yf - y;
    }
    else
    {
        x = y = w = h = 0;
    }
}

void Graph::draw( uint32_t *pixels, int width, int height )
{
    for( int i = 0; i < height; ++i )
    {
        for( int j = 0; j < width; ++j )
        {
            pixels[i * width + j] = 0xff555555;
        }
    }

    for( auto& node : nodes )
        node->draw( pixels, width, height );

    auto connect = [pixels, width, height]( Node & node, Node & output )
    {
        for( int index = 0; index < 4; ++index )
        {
            int cx = 0, cy = 0;
            rectanglePoint( node.x, node.y, node.w, node.h, index, cx, cy );

            for( int oindex = 0; oindex < 4; ++oindex )
            {
                int ocx = 0, ocy = 0;
                rectanglePoint( output.x, output.y, output.w, output.h, oindex, ocx, ocy );

                if( !vectorConner( cx, cy, ocx, ocy, index ) && !vectorConner( ocx, ocy, cx, cy, oindex ) )
                {
                    drawArrow( pixels, width, height, cx, cy, ocx, ocy, 2 );
                    return;
                }
            }
        }
    };

    for( auto& node : nodes )
    {
        unsigned n = node->outputSize();
        for( size_t i = 0; i < n; ++i )
            connect( *node, *node->output( i ) );
    }
}

Graph::Node* Graph::click( float x, float y )
{
    for( auto& v : nodes )
    {
        if( v->click( x, y ) )
            return v.get();
    }
    return nullptr;
}

bool Graph::cycle()
{
    for( auto& v : nodes )
        v->color = white;

    for( auto& v : nodes )
    {
        if( v->cycle() )
            return true;
    }

    return false;
}

bool Graph::sortAsTree( float sx, float sy )
{
    if( cycle() )
        return false;

    for( auto& v : nodes )
    {
        if( v->inputSize() > 0 )
            continue;

        v->sortAsTree( sx, sy );
    }

    return true;
}
