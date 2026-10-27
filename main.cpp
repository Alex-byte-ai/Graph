#include <filesystem>
#include <optional>

#include "CommandLineArguments.h"
#include "Image/Translate.h"
#include "Clipboard.h"
#include "Window.h"
#include "Basic.h"

#include "Graph.h"

static void Input( Graph& graph )
{
    static std::filesystem::path path( L"data/graph_files" );

    auto action = [&graph]( const std::optional<std::filesystem::path>& p )
    {
        if( !p )
            return;

        path = *p;

        auto ext = path.extension();
        if( ext == L".graph" )
        {
            graph.load( path );
            path = path.parent_path();
        }
        else if( ext == L".h" || ext == L".cpp" )
        {
            path = path.parent_path();
            graph.getCppStructure( path );
        }
    };

    auto arguments = commandLineArguments();
    if( arguments.size() > 1 )
    {
        path = arguments[1];
        action( path );
    }
    else
    {
        openPath( action, path );
    }
}

static void Output( Graph& graph )
{
    static std::filesystem::path path( L"data/graph_files/save" );

    auto action = [&graph]( const std::optional<std::filesystem::path>& p )
    {
        if( !p )
            return;

        path = *p;

        if( path.filename() != L"" )
        {
            graph.save( path );
            path = path.parent_path();
        }
    };

    savePath( action, path );
}

struct GraphWindow : public GraphicInterface::Window
{
    Graph::Node *lmb, *rmb, *mmb;
    Graph graph;
    int dx, dy;

    GraphWindow() : GraphicInterface::Window(), lmb( nullptr ), rmb( nullptr ), mmb( nullptr ), dx( 0 ), dy( 0 )
    {
        title.value = L"Graph";
        title.prepare();

        handleMsg = [this]( const GraphicInterface::InputData & input, GraphicInterface::OutputData & output )
        {
            auto& keyO = input.keys.letter( 'O' );
            auto& keyS = input.keys.letter( 'S' );
            auto& keyT = input.keys.letter( 'T' );
            auto& keyC = input.keys.letter( 'C' );
            auto& keyV = input.keys.letter( 'V' );

            auto update = [&]()
            {
                int w = input.width;
                int h = input.height;
                if( w <= 0 || h <= 0 )
                    return;

                auto image = new uint32_t[w * h];
                graph.draw( image, w, h );
                output.image.get().prepare( image, w, h );
                delete[] image;
            };

            if( input.scale || input.init || input.typed )
            {
                update();
                return true;
            }

            if( input.leftMouse.changed() )
            {
                if( *input.leftMouse )
                {
                    dx = *input.mouseX;
                    dy = *input.mouseY;
                    lmb = graph.click( dx, dy );
                    if( lmb )
                    {
                        dx = lmb->x - dx;
                        dy = lmb->y - dy;
                    }
                }
                else
                {
                    lmb = nullptr;
                }
                return true;
            }

            if( input.rightMouse.changed() )
            {
                if( *input.rightMouse )
                {
                    rmb = graph.click( *input.mouseX, *input.mouseY );
                    if( !rmb )
                    {
                        graph.add( 1 );
                        rmb = graph.get( graph.length() - 1 );
                        rmb->x = *input.mouseX;
                        rmb->y = *input.mouseY;
                        update();
                    }
                    return true;
                }
            }

            if( input.middleMouse.changed() )
            {
                if( *input.middleMouse )
                {
                    if( rmb )
                    {
                        mmb = graph.click( *input.mouseX, *input.mouseY );
                        if( mmb )
                        {
                            rmb->connect( mmb );
                            update();
                            return true;
                        }
                    }
                }
            }

            if( input.mouseX.changed() || input.mouseY.changed() )
            {
                if( lmb )
                {
                    lmb->x = *input.mouseX + dx;
                    lmb->y = *input.mouseY + dy;
                    update();
                    return true;
                }
            }

            if( input.escape.changed() && *input.escape )
            {
                output.quit = true;
                return true;
            }

            if( input.del.changed() && *input.del )
            {
                if( rmb )
                {
                    if( !rmb->image.empty() )
                    {
                        rmb->image.clear();
                        update();
                        return true;
                    }

                    rmb->detach();
                    rmb = nullptr;
                    update();
                    return true;
                }
            }

            if( *input.ctrl )
            {
                if( *keyO && keyO.changed() )
                {
                    Input( graph );
                    lmb = rmb = mmb = nullptr;
                    update();
                    return true;
                }
                if( *keyS && keyS.changed() )
                {
                    Output( graph );
                    return true;
                }
                if( *keyT && keyT.changed() )
                {
                    graph.sortAsTree( 24, 96 );
                    update();
                    return true;
                }
                if( rmb && *keyC && keyC.changed() )
                {
                    if( !rmb->image.empty() )
                    {
                        ImageConvert::Reference image;
                        image.reset = []( ImageConvert::Reference & )
                        {
                            return false;
                        };
                        image.clear = []( ImageConvert::Reference & )
                        {};
                        image.format = "B8G8R8A8";
                        image.w = rmb->w;
                        image.h = rmb->h;
                        image.link = rmb->image.data();
                        image.bytes = image.w * image.h * sizeof( rmb->image[0] );
                        Clipboard::Item item = std::move( image );
                        Clipboard::output( item );
                        return true;
                    }
                    Clipboard::Item item = rmb->text;
                    Clipboard::output( item );
                    return true;
                }
                if( rmb && *keyV && keyV.changed() )
                {
                    Clipboard::Item item;
                    Clipboard::input( item );
                    if( auto image = std::get_if<ImageConvert::Reference>( &item ) )
                    {
                        ImageConvert::Reference p;
                        p.reset = [this]( ImageConvert::Reference & ref )
                        {
                            if( ref.bytes > 0 )
                            {
                                rmb->image.resize( ( ref.bytes + 3 ) / 4 );
                                rmb->w = ref.w;
                                rmb->h = ref.h;
                                ref.link = rmb->image.data();
                            }
                            else
                            {
                                ref.link = nullptr;
                            }
                            return true;
                        };
                        p.clear = []( ImageConvert::Reference & )
                        {};
                        p.format = "B8G8R8A8*REPBG*REPRG*REPA255";
                        ImageConvert::translate( *image, p, false );
                        if( rmb->h < 0 )
                        {
                            auto link = rmb->image.data();
                            auto h = -rmb->h;
                            auto hh = h / 2;
                            auto w = rmb->w;
                            auto bytes = w * sizeof( rmb->image[0] );
                            rmb->h = h;

                            for( int i = 0; i < hh; ++i )
                                swap( link + ( i * w ), link + ( ( h - i - 1 ) * w ), bytes );
                        }
                        update();
                        return true;
                    }
                    if( auto text = std::get_if<std::wstring>( &item ) )
                    {
                        rmb->text += *text;
                        update();
                        return true;
                    }
                }
            }

            return false;
        };
    }

    virtual bool input( wchar_t c ) override
    {
        if( !rmb )
            return false;

        if( c == L'\b' )
        {
            rmb->text = rmb->text.substr( 0, rmb->text.length() - 1 );
            return true;
        }

        rmb->text += c;
        return true;
    }
};

int main()
{
    GraphWindow window;
    Input( window.graph );
    window.run( false );
    Output( window.graph );
    return 0;
}
