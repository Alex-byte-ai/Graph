#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Graph
{
public:
    enum Color
    {
        red,
        green,
        white,
        gray,
        black,
    };

    enum class Version : uint32_t
    {
        v0 = 0
    };

    class Node
    {
    private:
        std::vector<Node*> inputs;
        std::vector<Node*> outputs;

        Graph *root;
        unsigned id;
    public:
        std::vector<uint32_t> image;
        std::wstring text;
        int x, y, w, h;
        Color color;

        Node( Graph *parent, size_t n );

        bool connect( Node *o );
        Node* input( size_t i );
        Node* output( size_t i );

        size_t inputSize() const;
        size_t outputSize() const;
        size_t index() const;

        float widthFix( float sx, bool levelBelow = false, size_t count = -1 ) const;
        bool click( float x, float y ) const;

        void draw( uint32_t *pixels, int width, int height );

        bool cycle();
        void sortAsTree( float sx, float sy );

        std::shared_ptr<Node> detach();
        void remove( Node* o );
    };
private:
    Version version;
    std::vector<std::shared_ptr<Node>> nodes;
public:
    Graph();
    Graph( size_t n );
    ~Graph();

    void clear();
    void add( size_t n );

    size_t length() const;
    Node* get( size_t index );

    void getCppStructure( const std::filesystem::path& path );

    void save( const std::filesystem::path& path ) const;
    bool load( const std::filesystem::path& path );

    void boundingBox( int& x, int& y, int& w, int& h ) const;

    void draw( uint32_t *pixels, int width, int height );

    Node* click( float x, float y );

    bool cycle();
    bool sortAsTree( float sx, float sy );
};
