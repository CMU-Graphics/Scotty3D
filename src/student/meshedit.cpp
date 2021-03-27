
#include <queue>
#include <set>
#include <unordered_map>
#include <stdlib.h>
#include <iostream>

#include "../geometry/halfedge.h"
#include "debug.h"

/* Note on local operation return types:

    The local operations all return a std::optional<T> type. This is used so that your
    implementation can signify that it does not want to perform the operation for
    whatever reason (e.g. you don't want to allow the user to erase the last vertex).

    An optional can have two values: std::nullopt, or a value of the type it is
    parameterized on. In this way, it's similar to a pointer, but has two advantages:
    the value it holds need not be allocated elsewhere, and it provides an API that
    forces the user to check if it is null before using the value.

    In your implementation, if you have successfully performed the operation, you can
    simply return the required reference:

            ... collapse the edge ...
            return collapsed_vertex_ref;

    And if you wish to deny the operation, you can return the null optional:

            return std::nullopt;

    Note that the stubs below all reject their duties by returning the null optional.
*/

/*
    This method should replace the given vertex and all its neighboring
    edges and faces with a single face, returning the new face.
 */
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::erase_vertex(Halfedge_Mesh::VertexRef v) {

    //(void)v;
    //return std::nullopt;
    if (v->on_boundary()) {
        return v->halfedge()->face();
    }
    HalfedgeRef h = v->halfedge();
    HalfedgeRef hn = v->halfedge()->next();
    do {
        h = h->twin()->next();
        erase_edge(h->edge());
    } while (h != v->halfedge());
    return hn->face();
}

/*
    This method should erase the given edge and return an iterator to the
    merged face.
 */
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::erase_edge(Halfedge_Mesh::EdgeRef e) {

    //(void)e;
    //return std::nullopt;
    if (e->on_boundary()) {
        return e->halfedge()->face();
    }

    if(e->halfedge()->face() == e->halfedge()->twin()->face()) {
        if(e->halfedge()->next()->next() == e->halfedge() && e->halfedge()->next() == e->halfedge()->twin()) {
            erase(e->halfedge()->twin()->vertex());
            erase(e->halfedge()->vertex());
            erase(e->halfedge()->face());
            erase(e->halfedge()->twin());
            erase(e->halfedge());
            erase(e);
            return faces.end();
        }
        HalfedgeRef h = (e->halfedge()->next() == e->halfedge()->twin()) ? e->halfedge() : e->halfedge()->twin();
        HalfedgeRef ht = h->twin();
        HalfedgeRef pre = h;
        do {
            pre = pre->next();
        } while(pre->next() != h);
        HalfedgeRef nex = ht->next();
        VertexRef v0 = h->vertex();
        VertexRef v1 = ht->vertex();
        FaceRef f = h->face();

        if(v0->halfedge() == h) {
            v0->halfedge() = nex;
        }

        if(f->halfedge() == h || f->halfedge() == ht) {
            f->halfedge() = pre;
        }
        pre->next() = nex;

        erase(v1);
        erase(h);
        erase(ht);
        erase(e);
        return f;

    }
    std::vector<HalfedgeRef> hlist;
    HalfedgeRef h = e->halfedge();
    HalfedgeRef start = h;
    int count = 0;
    int start2;
    do {
        start = start->next();
        count++;
        hlist.push_back(start);
    } while (start->next() != h);
    start2 = count;
    HalfedgeRef ht = e->halfedge()->twin();
    start = ht;
    do {
        start = start->next();
        count++;
        hlist.push_back(start);
    } while(start->next() != ht);

    FaceRef f = e->halfedge()->face();
    FaceRef f2 = e->halfedge()->twin()->face();
    hlist[start2-1]->next() = hlist[start2];
    hlist[start2]->vertex()->halfedge() = hlist[start2];
    hlist[count-1]->next() = hlist[0];
    hlist[0]->vertex()->halfedge() = hlist[0];

    for (auto half : hlist) {
        half->face() = f;
    }
    f->halfedge() = hlist[0];
    erase(e);
    erase(e->halfedge()->twin());
    erase(e->halfedge());
    erase(f2);
    
    return f;
}

/*
    This method should collapse the given edge and return an iterator to
    the new vertex created by the collapse.
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::collapse_edge(Halfedge_Mesh::EdgeRef e) {

    //(void)e;
    //return std::nullopt;
    std::cerr << "Collapsing " << e->id() << std::endl;
    //std::cerr << "C_h " << e->halfedge()->id() << std::endl;
    //std::cerr << "C_next " << e->halfedge()->next()->id() << std::endl;
    //std::cerr << "C_next_next " << e->halfedge()->next()->next()->id() << std::endl;
 
    /*for (Face f : faces) {
        std::cerr << "Face: " << f.id() << std::endl;
    }*/
    if(e->on_boundary()) {
        HalfedgeRef h = (e->halfedge()->is_boundary()) ? (e->halfedge()->twin()) : (e->halfedge());
        HalfedgeRef ht = h->twin();
        FaceRef bound = h->twin()->face();
        VertexRef v1 = h->vertex();
        VertexRef v2 = ht->vertex();
        Vec3 vpos = (v1->pos + v2->pos)/2;
        std::vector<HalfedgeRef> hlist;
        int count = 0;
        HalfedgeRef start = h;
        do {
            start = start->next();
            count++;
            hlist.push_back(start);
        } while (start->next() != h);
        start = ht->twin()->next();
        
        
        if(count == 2) {
            if (hlist[0]->edge()->on_boundary() && hlist[1]->edge()->on_boundary()) {
                return e->halfedge()->vertex();
            }
            HalfedgeRef toptw = hlist[0]->twin();
            HalfedgeRef bottw = hlist[1]->twin();
            EdgeRef e0 = toptw->edge();
            EdgeRef e1 = bottw->edge();
            VertexRef vx = toptw->vertex();
            FaceRef f = hlist[1]->face();
            if (toptw == hlist[1]) return std::nullopt;
            toptw->twin() = bottw;
            bottw->twin() = toptw;
            toptw->edge() = e0;
            bottw->edge() = e0;
            bottw->vertex() = v1;
            e0->halfedge() = toptw;
            std::cerr << "BoundTop HE erase: " << hlist[0]->id() << std::endl;
            erase(hlist[0]);
            std::cerr << "BoundBot HE erase: " << hlist[1]->id() << std::endl;
            erase(hlist[1]);
            std::cerr << "Bound Edge erase: " << e1->id() << std::endl;
            erase(e1);
            std::cerr << "Bound Face erase: " << f->id() << std::endl;
            erase(f);
            v1->halfedge() = bottw;
            std::cerr << "Vert " << vx->id() << "; HE: " << vx->halfedge()->id() << std::endl;
            vx->halfedge() = toptw;

        } else {
            hlist[count-1]->next() = hlist[0];
            hlist[0]->vertex() = v1;
            v1->halfedge() = hlist[0];
        }
        do {
            start->vertex() = v1;
            start = start->twin()->next();
        } while (start != ht);
        HalfedgeRef pre = h->twin();
        do {
            pre = pre->next();
        } while (pre->next() != h->twin());
        pre->next() = h->twin()->next();
        bound->halfedge() = pre;
        std::cerr << "HE erase: " << h->id() << std::endl;
        erase(h);
        std::cerr << "HET erase: " << h->twin()->id() << std::endl;
        erase(h->twin());
        std::cerr << "edge erase: " << e->id() << std::endl;
        erase(e);
        std::cerr << "vertex erase: " << v2->id() << std::endl;
        erase(v2);
        v1->pos = vpos;
        return v1;

    }
    //return e->halfedge()->vertex();
    HalfedgeRef h = e->halfedge();
    HalfedgeRef ht = e->halfedge()->twin();
    //VertexRef v = new_vertex();
    Vec3 vpos(0, 0, 0);
    VertexRef v1 = h->vertex();
    VertexRef v2 = ht->vertex();
    std::cerr << "v1 original pos: " << v1->pos << std::endl;
    std::cerr << "v2 original pos: " << v2->pos << std::endl;
    if(isnan(v1->pos.x) || isnan(v1->pos.y) || isnan(v1->pos.z)) return std::nullopt;
    if(isnan(v2->pos.x) || isnan(v2->pos.y) || isnan(v2->pos.z)) return std::nullopt;
    
    vpos = (v1->pos + v2->pos)/2;
    //std::cerr << "new vpos: " << vpos << std::endl;
    
    
    std::vector<HalfedgeRef> left;
    std::vector<HalfedgeRef> right;
    HalfedgeRef start = h;
    int lcount = 0;
    int rcount = 0;
    do {
        start = start->next();
        lcount++;
        left.push_back(start);
    } while (start->next() != h);
    start = ht;
    do {
        start = start->next();
        rcount++;
        right.push_back(start);
    } while (start->next() != ht);
    //return v2;
    /*if (lcount == 2) {
        if(left[1]->vertex()->degree() == 3) {
            return e->halfedge()->vertex();
        }
    }
    if (rcount == 2) {
        if(right[1]->vertex()->degree() == 3) {
            return e->halfedge()->vertex();
        }
    }*/
    if(left[0]->edge()->on_boundary() && left[lcount-1]->edge()->on_boundary()) {
        return e->halfedge()->vertex();
    }
    if(right[0]->edge()->on_boundary() && right[rcount-1]->edge()->on_boundary()) {
        return e->halfedge()->vertex();
    }
    //std::cerr << "l0t " << left[0]->twin()->id() << "; l1 " << left[1]->id() << std::endl;
    if (left[0]->twin() == left[1]) return std::nullopt;
    //std::cerr << "l0t " << left[0]->twin()->id() << "; r1 " << right[1]->id() << std::endl;
    if (left[0]->twin() == right[1]) return std::nullopt;
    //if (left[1] == left[0]->twin()) return std::nullopt;
    //std::cerr << "r0t " << right[0]->twin()->id() << "; r1 " << right[1]->id() << std::endl;
    if (right[1] == right[0]->twin()) return std::nullopt;
    //std::cerr << "right[1] = " << right[1]->id() << "; right[0]-twin = " << right[0]->twin()->id() << std::endl;
    HalfedgeRef top = ht->twin()->next();
    do {
        top->vertex() = v1;
        top = top->twin()->next();
    } while (top != ht);
    

    if(lcount == 2) {
        HalfedgeRef toptw = left[0]->twin();
        HalfedgeRef bottw = left[1]->twin();
        VertexRef vx = toptw->vertex();
        FaceRef f = left[1]->face();
        EdgeRef e0 = left[0]->edge();
        EdgeRef e1 = left[1]->edge();
        //std::cerr << "toptw = " << toptw->id() << "; left[1] = " << left[1]->id() << std::endl;
        //if(toptw == left[1]) 
        //std::cerr << "toptwr = " << toptw->id() << "; right[1] = " << right[1]->id() << std::endl;

        toptw->twin() = bottw;
        bottw->twin() = toptw;
        toptw->edge() = e0;
        bottw->edge() = e0;
        bottw->vertex() = v1;
        e0->halfedge() = toptw;
        //HalfedgeRef pretop = toptw;
        /*do {
            pretop = pretop->next();
        } while (pretop->next() != toptw);
        //std::cerr << "Pretop: " << pretop->id() << "; pretop next: " << pretop->next()->id() << std::endl;
        //std::cerr << "Nextop: " << toptw->next()->id() << "; nextop next: " << toptw->next()->next()->id() << std::endl;
        HalfedgeRef prebot = bottw;
        do {
            prebot = prebot->next();
        } while (prebot->next() != bottw);*/
        //std::cerr << "Prebot: " << prebot->id() << "; prebot next: " << prebot->next()->id() << std::endl;
        //std::cerr << "Nexbot: " << bottw->next()->id() << "; nexbot next: " << bottw->next()->next()->id() << std::endl;
        //std::cerr << "Vert Pre" << vx->id() << "; HE: " << vx->halfedge()->id() << std::endl;
        //std::cerr << "Left Face erase: " << f->id() << std::endl;

        erase(f);
        //std::cerr << "Left HE0 erase: " << left[0]->id() << std::endl;
        erase(left[0]);
        //std::cerr << "Left HE1 erase: " << left[1]->id() << std::endl;
        erase(left[1]);
        //std::cerr << "Left Edge erase: " << e1->id() << std::endl;
        erase(e1);
        v1->halfedge() = bottw;
        vx->halfedge() = toptw;
        //std::cerr << "Vert Post" << vx->id() << "; HE: " << vx->halfedge()->id() << std::endl;
    } else {
        left[lcount-1]->next() = left[0];
        left[0]->vertex() = v1;
        v1->halfedge() = left[0];
        left[0]->face()->halfedge() = left[0];
    }

    if(rcount == 2) {
        HalfedgeRef toptw = right[0]->twin();
        HalfedgeRef bottw = right[1]->twin();
        VertexRef vx = toptw->vertex();
        FaceRef f = right[1]->face();
        EdgeRef e0 = right[0]->edge();
        EdgeRef e1 = right[1]->edge();
        //std::cerr << "toptw = " << toptw->id() << "; right[1] = " << right[1]->id() << std::endl;
        //if(toptw == right[1])
        toptw->twin() = bottw;
        bottw->twin() = toptw;
        toptw->edge() = e0;
        bottw->edge() = e0;
        bottw->vertex() = v1;
        e0->halfedge() = toptw;
        //HalfedgeRef pretop = toptw;
        /*do {
            pretop = pretop->next();
        } while (pretop->next() != toptw);
        //std::cerr << "Pretop: " << pretop->id() << "; pretop next: " << pretop->next()->id() << std::endl;
        //std::cerr << "Nextop: " << toptw->next()->id() << "; nextop next: " << toptw->next()->next()->id() << std::endl;
        HalfedgeRef prebot = bottw;
        do {
            prebot = prebot->next();
        } while (prebot->next() != bottw);*/
        //std::cerr << "Prebot: " << prebot->id() << "; prebot next: " << prebot->next()->id() << std::endl;
        //std::cerr << "Nexbot: " << bottw->next()->id() << "; nexbot next: " << bottw->next()->next()->id() << std::endl;
        //std::cerr << "Vert Pre" << vx->id() << "; HE: " << vx->halfedge()->id() << std::endl;
        //std::cerr << "Right Face erase: " << f->id() << std::endl;

        erase(f);
        //std::cerr << "Right HE0 erase: " << right[0]->id() << std::endl;
        erase(right[0]);
        //std::cerr << "Right HE1 erase: " << right[1]->id() << std::endl;
        erase(right[1]);
        //std::cerr << "Right Edge erase: " << e1->id() << std::endl;
        erase(e1);
        v1->halfedge() = bottw;
        vx->halfedge() = toptw;
        //std::cerr << "Vert Post" << vx->id() << "; HE: " << vx->halfedge()->id() << std::endl;
    } else {
        right[rcount-1]->next() = right[0];
        right[0]->vertex() = v1;
        right[0]->face()->halfedge() = right[0];
    }
    
    //std::cerr << "HE erase: " << h->id() << std::endl;
    erase(h);
    //std::cerr << "HET erase: " << h->twin()->id() << std::endl;
    erase(h->twin());
    //std::cerr << "vertex erase: " << h->twin()->vertex()->id() << std::endl;
    erase(h->twin()->vertex());
    std::cerr << "Edge erase: " << e->id() << std::endl;
    erase(e);
    //std::cerr << "V1 new pos: " << vpos << std::endl;
    v1->pos = vpos;
    
    return v1;

    
}

/*
    This method should collapse the given face and return an iterator to
    the new vertex created by the collapse.
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::collapse_face(Halfedge_Mesh::FaceRef f) {

    //(void)f;
    //return std::nullopt;
    if(f->is_boundary()) {
        return f->halfedge()->vertex();
    }
    VertexRef mid = new_vertex();
    mid->pos = f->center();
    std::vector<HalfedgeRef> hlist;
    int count = 0;
    HalfedgeRef start = f->halfedge();
    do {
        hlist.push_back(start);
        count++;
        start = start->next();
    } while (start != f->halfedge());

    for(int i = 0; i < count; i++) {
        HalfedgeRef oppface = hlist[i]->twin();
        HalfedgeRef a = oppface->next();
        HalfedgeRef b = oppface;
        //VertexRef apoint = a->vertex();
        FaceRef face = a->face();
        do {
            b = b->next();
        } while (b->next() != oppface);
        b->next() = a;
        a->vertex() = mid;
        b->vertex() = b->vertex();
        b->twin()->vertex() = mid;
        mid->halfedge() = a;
        face->halfedge() = a;
    }
    for (auto h : hlist) {
        erase(h);
        erase(h->twin());
        erase(h->edge());
        erase(h->vertex());
    }
    erase(f);
    return mid;
}

/*
    This method should flip the given edge and return an iterator to the
    flipped edge.
*/
std::optional<Halfedge_Mesh::EdgeRef> Halfedge_Mesh::flip_edge(Halfedge_Mesh::EdgeRef e) {

    //(void)e;
    //return std::nullopt;
    if(e->on_boundary()) {
        return e;
    }
    std::vector<HalfedgeRef> hlist;
    std::vector<HalfedgeRef> twinlist;
    size_t count = 0;
    size_t start2;
    HalfedgeRef h = e->halfedge();
    HalfedgeRef start = h;
    do {
        start = start->next();
        count++;
        hlist.push_back(start);
        twinlist.push_back(start->twin());
    } while (start->next() != h);
    start2 = hlist.size();
    HalfedgeRef ht = e->halfedge()->twin();
    start = ht;
    do {
        start = start->next();
        count++;
        hlist.push_back(start);
        twinlist.push_back(start->twin());
    } while (start->next() != ht);
    count = hlist.size();
    VertexRef v0 = hlist[1]->vertex();
    VertexRef v1 = hlist[start2+1]->vertex();
    VertexRef v2 = h->vertex();
    VertexRef v3 = ht->vertex();
    FaceRef f0 = h->face();
    FaceRef f1 = ht->face();

    h->next() = hlist[1];
    hlist[start2-1]->next() = hlist[start2];
    hlist[start2]->next() = h;

    ht->next() = hlist[start2+1];
    hlist[count-1]->next() = hlist[0];
    hlist[0]->next() = ht;

    h->vertex() = v1;
    ht->vertex() = v0;
    
    v2->halfedge() = hlist[start2];
    v3->halfedge() = hlist[0];
    //f0->halfedge() = h;
    //f1->halfedge() = ht;
    h->face() = f0;
    ht->face() = f1;
    for(size_t i = 1; i <= start2; i++) {
        hlist[i]->face() = h->face();
        twinlist[i]->twin() = hlist[i];
        twinlist[i]->face() = twinlist[i]->face();
        hlist[i]->next() = (i==start2)? h : hlist[i+1];
    }
    for(size_t i = start2+1; i < count; i++) {
        hlist[i]->face() = ht->face();
        twinlist[i]->twin() = hlist[i];
        twinlist[i]->face() = twinlist[i]->face();
        hlist[i]->next() = (i==count-1)? hlist[0] : hlist[i+1]; 
    }
    hlist[0]->face() = ht->face();
    hlist[start2]->face() = h->face();
    twinlist[0]->twin() = hlist[0];
    twinlist[0]->face() = twinlist[0]->face();
    h->face()->halfedge() = h;
    ht->face()->halfedge() = ht;
    return e;

}
/*
    This method bisects the given edge and returns an iterator to the
    newly inserted vertex.
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::bisect_edge(Halfedge_Mesh::EdgeRef e) {
    
    HalfedgeRef h = (e->halfedge()->is_boundary()) ? e->halfedge()->twin() : e->halfedge();
    HalfedgeRef ht = h->twin();
    HalfedgeRef preh = h;
    HalfedgeRef nexht = ht->next();
    do {
        preh = preh->next();
    } while (preh->next() != h);
    Vec3 vpos = (h->vertex()->pos + ht->vertex()->pos)/2;
    VertexRef c = new_vertex();
    c->pos = vpos;
    HalfedgeRef hn = new_halfedge();
    HalfedgeRef hnt = new_halfedge();
    EdgeRef e0 = new_edge();
    e0->halfedge() = hn;
    hn->twin() = hnt;
    hnt->twin() = hn;
    hn->edge() = e0;
    hnt->edge() = e0;
    hn->vertex() = h->vertex();
    hnt->vertex() = c;
    hn->face() = h->face();
    hnt->face() = ht->face();
    preh->next() = hn;
    hn->next() = h;
    h->vertex() = c;
    ht->next() = hnt;
    hnt->next() = nexht;
    c->halfedge() = h;
    hn->vertex()->halfedge() = hn;
    c->is_new = true;
    return c;
}



/*
    This method should split the given edge and return an iterator to the
    newly inserted vertex. The halfedge of this vertex should point along
    the edge that was split, rather than the new edges.
*/
std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::split_edge(Halfedge_Mesh::EdgeRef e) {

    //(void)e;
    //return std::nullopt;
    if (e->on_boundary() && e->halfedge()->face()->degree() > 3) {
        return e->halfedge()->vertex();
    }
    if (e->on_boundary()) {
        HalfedgeRef h = (e->halfedge()->is_boundary()) ? e->halfedge()->twin() : e->halfedge();
        VertexRef c = new_vertex();
        FaceRef f1 = h->face();
        VertexRef v1 = h->vertex();
        VertexRef v2 = h->twin()->vertex();
        c->pos = (v1->pos + v2->pos)/2;
        HalfedgeRef h0 = new_halfedge();
        HalfedgeRef h0t = new_halfedge();
        HalfedgeRef h_0 = h->next();
        HalfedgeRef h_1 = h_0->next();
        VertexRef v0 = h_1->vertex();
        HalfedgeRef twinnext = h->twin()->next();
        EdgeRef e0 = new_edge();
        c->is_new = true;
        e0->is_new = false;
        e->is_new = false;
        h0->twin() = h0t;
        h0t->twin() = h0;
        e0->halfedge() = h0;
        h0->edge() = e0;
        h0t->edge() = e0;
        h0->vertex() = v1;
        h0t->vertex() = c;
        h0t->face() = h->twin()->face();
        h0->face() = f1;
        h0->next() = h;
        h_1->next() = h0;
        h->vertex() = c;
        h->twin()->next() = h0t;
        h0t->next() = twinnext;
        c->halfedge() = h;
        

        FaceRef f2 = new_face();
        EdgeRef e2 = new_edge();
        HalfedgeRef h1 = new_halfedge();
        HalfedgeRef h1t = new_halfedge();
        e2->is_new = true;
        h1->twin() = h1t;
        h1t->twin() = h1;
        h1->edge() = e2;
        h1t->edge() = e2;
        e2->halfedge() = h1;
        h1->face() = f1;
        h1t->face() = f2;
        f2->halfedge() = h1t;
        h1->vertex() = v0;
        h1t->vertex() = c;
        h_0->next() = h1;
        h1->next() = h;
        h1t->next() = h_1;
        h0->next() = h1t;
        h_1->face() = f2;
        h0->face() = f2;
        h_0->vertex() = v2;
        h->face() = f1;
        h_0->face() = f1;
        f1->halfedge() = h1;
        f2->halfedge() = h1t;

        HalfedgeRef start = c->halfedge();
        do {
            start = start->twin()->next();
            start->vertex() = c;
        } while (start != c->halfedge());
        return c;

    }
    if(e->halfedge()->face()->degree() > 3 || e->halfedge()->twin()->face()->degree() > 3) {
        return e->halfedge()->vertex();
    }
    
    HalfedgeRef h = e->halfedge();
    HalfedgeRef ht = e->halfedge()->twin();
    std::vector<HalfedgeRef> hlist;
    Vec3 v1pos = h->vertex()->pos;
    Vec3 v2pos = ht->vertex()->pos;
    VertexRef v1 = h->vertex();
    VertexRef v2 = ht->vertex();
    FaceRef f1 = h->face();
    FaceRef f2 = ht->face();
    VertexRef c = new_vertex();
    c->pos = (v1pos+v2pos)/2;
    std::cerr << "Split vpos check: " << c->pos << std::endl;
    HalfedgeRef h2 = new_halfedge();
    HalfedgeRef h2t = new_halfedge();
    EdgeRef e2 = new_edge();
    c->is_new = true;
    e2->is_new = false;
    e->is_new = false;
    h2->twin() = h2t;
    h2t->twin() = h2;
    e2->halfedge() = h2;
    std::cerr << "new edge: " << e2->id() << std::endl;
    h2->vertex() = c;
    h2t->vertex() = v1;
    h2->edge() = e2;
    h2t->edge() = e2;
    h2t->face() = f1;
    h2->face() = f2;
    int count = 0;
    int start2;
    HalfedgeRef start = h;
    do {
        start = start->next();
        count++;
        hlist.push_back(start);
    } while (start->next() != h);
    start2 = count;
    start = ht;
    do {
        start = start->next();
        count++;
        hlist.push_back(start);
    } while (start->next() != ht);

    hlist[start2-1]->next() = h2t;
    h2t->next() = h;
    h2->next() = hlist[start2];
    ht->next() = h2;
    h->vertex() = c;
    ht->vertex() = v2;
    c->halfedge() = h;
    v1->halfedge() = h2t;
    //return c;


    EdgeRef e3 = new_edge();
    HalfedgeRef h3 = new_halfedge();
    HalfedgeRef h3t = new_halfedge();
    FaceRef f3 = new_face();
    e3->is_new = true;
    h3->twin() = h3t;
    h3t->twin() = h3;
    e3->halfedge() = h3;
    std::cerr << "new edge: " << e3->id() << std::endl;
    h3->edge() = e3;
    h3t->edge() = e3;
    h3t->face() = f3;
    h3->vertex() = c;
    h3t->vertex() = hlist[count-1]->vertex();
    f3->halfedge() = h3t;
    h3->face() = f2;
    hlist[start2]->next() = h3t;
    h3t->next() = h2;
    ht->next() = h3;
    h3->next() = hlist[count-1];
    h2->face() = f3;
    hlist[start2]->face() = f3;
    f2->halfedge() = ht;
    
    EdgeRef e4 = new_edge();
    HalfedgeRef h4 = new_halfedge();
    HalfedgeRef h4t = new_halfedge();
    FaceRef f4 = new_face();
    e4->is_new = true;
    h4->twin() = h4t;
    h4t->twin() = h4;
    e4->halfedge() = h4;
    std::cerr << "new edge: " << e4->id() << std::endl;
    h4->edge() = e4;
    h4t->edge() = e4;
    h4t->face() = f1;
    h4->vertex() = c;
    h4t->vertex() = hlist[start2-1]->vertex();
    f1->halfedge() = h;
    h4->face() = f4;
    hlist[0]->next() = h4t;
    h4t->next() = h;
    h2t->next() = h4;
    h4->next() = hlist[start2-1];
    h2t->face() = f4;
    hlist[start2-1]->face() = f4;
    f4->halfedge() = h4;
    return c;
        
}

/* Note on the beveling process:

    Each of the bevel_vertex, bevel_edge, and bevel_face functions do not represent
    a full bevel operation. Instead, they should update the _connectivity_ of
    the mesh, _not_ the positions of newly created vertices. In fact, you should set
    the positions of new vertices to be exactly the same as wherever they "started from."

    When you click on a mesh element while in bevel mode, one of those three functions
    is called. But, because you may then adjust the distance/offset of the newly
    beveled face, we need another method of updating the positions of the new vertices.

    This is where bevel_vertex_positions, bevel_edge_positions, and
    bevel_face_positions come in: these functions are called repeatedly as you
    move your mouse, the position of which determins the normal and tangent offset
    parameters. These functions are also passed an array of the original vertex
    positions: for  bevel_vertex, it has one element, the original vertex position,
    for bevel_edge,  two for the two vertices, and for bevel_face, it has the original
    position of each vertex in halfedge order. You should use these positions, as well
    as the normal and tangent offset fields to assign positions to the new vertices.

    Finally, note that the normal and tangent offsets are not relative values - you
    should compute a particular new position from them, not a delta to apply.
*/

/*
    This method should replace the vertex v with a face, corresponding to
    a bevel operation. It should return the new face.  NOTE: This method is
    responsible for updating the *connectivity* of the mesh only---it does not
    need to update the vertex positions.  These positions will be updated in
    Halfedge_Mesh::bevel_vertex_positions (which you also have to
    implement!)
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_vertex(Halfedge_Mesh::VertexRef v) {

    // Reminder: You should set the positions of new vertices (v->pos) to be exactly
    // the same as wherever they "started from."

    //(void)v;
    //return std::nullopt;
    std::vector<HalfedgeRef> hlist, nhlist;
    std::vector<VertexRef> vlist;
    HalfedgeRef start = v->halfedge();
    FaceRef newf = new_face();
    int count = 0;
    do {
        hlist.push_back(start);
        VertexRef nv = new_vertex();
        nv->pos = v->pos;
        vlist.push_back(nv);
        count++;
        start = start->twin()->next();
    } while (start != v->halfedge());
    for (int i = 0; i < count; i++) {
        int indplus = (i+count-1)%count;
        HalfedgeRef a = hlist[i];
        HalfedgeRef b = hlist[indplus]->twin();
        HalfedgeRef newh = new_halfedge();
        HalfedgeRef newht = new_halfedge();
        EdgeRef newe = new_edge();
        newh->twin() = newht;
        newht->twin() = newh;
        newh->edge() = newe;
        newht->edge() = newe;
        newe->halfedge() = newh;
        newh->face() = a->face();
        newht->face() = newf;
        newh->vertex() = vlist[indplus];
        newht->vertex() = vlist[i];
        vlist[i]->halfedge() = newht;
        nhlist.push_back(newht);
        b->next() = newh;
        newh->next() = a;
        a->vertex() = vlist[i];
    }
    for (int i = 0; i < count; i++) {
        int indplus = (i+count-1)%count;
        nhlist[i]->next() = nhlist[indplus];
    }
    newf->halfedge() = nhlist[0];
    erase(v);
    return newf;

    

}

/*
    This method should replace the edge e with a face, corresponding to a
    bevel operation. It should return the new face. NOTE: This method is
    responsible for updating the *connectivity* of the mesh only---it does not
    need to update the vertex positions.  These positions will be updated in
    Halfedge_Mesh::bevel_edge_positions (which you also have to
    implement!)
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_edge(Halfedge_Mesh::EdgeRef e) {

    // Reminder: You should set the positions of new vertices (v->pos) to be exactly
    // the same as wherever they "started from."

    //(void)e;
    //return std::nullopt;
    if(e->on_boundary()) {
        return std::nullopt;
    }
    std::vector<HalfedgeRef> topH, botH;
    std::vector<HalfedgeRef> htoplist, hbotlist;
    std::vector<VertexRef> topv, botv;
    HalfedgeRef start = e->halfedge();
    VertexRef vref = start->vertex();
    FaceRef newf = new_face();
    do {
        start = start->twin()->next();
        VertexRef newv = new_vertex();
        newv->pos = vref->pos;
        //start->vertex() = newv;
        newv->halfedge() = start;
        botv.push_back(newv);
        botH.push_back(start);
    } while (start->twin()->next() != e->halfedge());
    start = e->halfedge()->twin();
    vref = start->vertex();
    do {
        start = start->twin()->next();
        VertexRef newv = new_vertex();
        newv->pos = vref->pos;
        //start->vertex() = newv;
        newv->halfedge() = start;
        topv.push_back(newv);
        topH.push_back(start);
    } while (start->twin()->next() != e->halfedge()->twin());

    for(size_t i = 1; i < botH.size(); i++) {
        HalfedgeRef newh = new_halfedge();
        HalfedgeRef newht = new_halfedge();
        EdgeRef newe = new_edge();
        HalfedgeRef a = botH[i];
        HalfedgeRef b = botH[i-1]->twin();
        FaceRef f = a->face();
        newh->twin() = newht;
        newht->twin() = newh;
        newe->halfedge() = newh;
        newh->edge() = newe;
        newht->edge() = newe;
        newh->vertex() = botv[i-1];
        newht->vertex() = botv[i];
        b->next() = newh;
        newh->next() = a;
        b->face() = f;
        newh->face() = f;
        newht->face() = newf;
        a->face() = f;
        f->halfedge() = a;
        a->vertex() = botv[i];
        botv[i]->halfedge() = newht;
        botv[i-1]->halfedge() = newh;
        hbotlist.push_back(newht);
    }
    for(size_t i = 1; i < topH.size(); i++) {
        HalfedgeRef newh = new_halfedge();
        HalfedgeRef newht = new_halfedge();
        EdgeRef newe = new_edge();
        HalfedgeRef a = topH[i];
        HalfedgeRef b = topH[i-1]->twin();
        FaceRef f = a->face();
        newh->twin() = newht;
        newht->twin() = newh;
        newe->halfedge() = newh;
        newh->edge() = newe;
        newht->edge() = newe;
        newh->vertex() = topv[i-1];
        newht->vertex() = topv[i];
        b->next() = newh;
        newh->next() = a;
        b->face() = f;
        newh->face() = f;
        newht->face() = newf;
        a->face() = f;
        f->halfedge() = a;
        a->vertex() = topv[i];
        htoplist.push_back(newht);
        topv[i-1]->halfedge() = newh;
        topv[i]->halfedge() = newht;
    }
    HalfedgeRef newh0 = new_halfedge();
    HalfedgeRef newht0 = new_halfedge();
    EdgeRef newe0 = new_edge();
    HalfedgeRef a0 = botH[0];
    HalfedgeRef b0 = topH[topH.size()-1]->twin();
    FaceRef f0 = a0->face();
    newh0->twin() = newht0;
    newht0->twin() = newh0;
    newe0->halfedge() = newh0;
    newh0->edge() = newe0;
    newht0->edge() = newe0;
    newh0->vertex() = topv[topv.size()-1];
    newht0->vertex() = botv[0];
    b0->next() = newh0;
    newh0->next() = a0;
    b0->face() = f0;
    newh0->face() = f0;
    newht0->face() = newf;
    a0->face() = f0;
    f0->halfedge() = a0;
    a0->vertex() = botv[0];
    botv[0]->halfedge() = newht0;
    topv[topv.size()-1]->halfedge() = newh0;

    HalfedgeRef newh1 = new_halfedge();
    HalfedgeRef newht1 = new_halfedge();
    EdgeRef newe1 = new_edge();
    HalfedgeRef a1 = topH[0];
    HalfedgeRef b1 = botH[botH.size()-1]->twin();
    FaceRef f1 = a1->face();
    newh1->twin() = newht1;
    newht1->twin() = newh1;
    newe1->halfedge() = newh1;
    newh1->edge() = newe1;
    newht1->edge() = newe1;
    newh1->vertex() = botv[botv.size()-1];
    newht1->vertex() = topv[0];
    b1->next() = newh1;
    newh1->next() = a1;
    b1->face() = f1;
    newh1->face() = f1;
    newht1->face() = newf;
    a1->face() = f1;
    f1->halfedge() = a1;
    a1->vertex() = topv[0];
    topv[0]->halfedge() = newht1;
    botv[botv.size()-1]->halfedge() = newh1;

    for(size_t i = hbotlist.size()-1; i > 0; i--) {
        HalfedgeRef curr = hbotlist[i];
        curr->next() = hbotlist[i-1];
        curr->face() = newf;
    }
    hbotlist[0]->next() = newht0;
    hbotlist[0]->face() = newf;
    newht0->next() = htoplist[htoplist.size()-1];
    for(size_t i = htoplist.size()-1; i > 0; i--) {
        HalfedgeRef curr = htoplist[i];
        curr->next() = htoplist[i-1];
        curr->face() = newf;
    }
    htoplist[0]->next() = newht1;
    htoplist[0]->face() = newf;
    newht1->next() = hbotlist[hbotlist.size()-1];
    newf->halfedge() = hbotlist[0];
    erase(e);
    erase(e->halfedge());
    erase(e->halfedge()->twin());
    erase(e->halfedge()->vertex());
    erase(e->halfedge()->twin()->vertex());
    return newf;

}


std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::extrude_vertex(Halfedge_Mesh::VertexRef v) {
    if(v->on_boundary()) return std::nullopt;
    auto ref = bevel_vertex(v);
    if (!ref.has_value()) {
        return std::nullopt;
    }
    FaceRef f = ref.value();
    Vec3 normal (0,0,0);
    HalfedgeRef start = f->halfedge();
    int degree = 0;
    float edgeMean = 0.0f;
    Vec3 startPos = v->pos;
    extrude_vertex_positions(startPos, f);
    normal = -f->normal();
    do {
        edgeMean += start->edge()->length();
        start = start->next();
        degree++;
    } while (start != f->halfedge());
    
    
    edgeMean *= 1.0f/degree;
    
    auto ref2 = insert_vertex(f);
    if (!ref2.has_value()) {
        return std::nullopt;
    }
    VertexRef v2 = ref2.value();
    std::cerr << "edgeMean: " << edgeMean << std::endl;
    std::cerr << "normal: " << normal << std::endl;
    v2->pos += normal * edgeMean;
    std::cerr << "v2pos: " << v2->pos << std::endl;
    return v2;

}

std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::inset_face(Halfedge_Mesh::FaceRef f) {
    
    std::vector<Vec3> vpos;
    std::vector<HalfedgeRef> hlist;
    auto ref = bevel_face(f);
    if (!ref.has_value()) return std::nullopt;
    FaceRef f2 = ref.value();
    HalfedgeRef h = f2->halfedge();
    HalfedgeRef start = h;
    do {
        hlist.push_back(start);
        vpos.push_back(start->vertex()->pos);
        start = start->next();
    } while (start != f2->halfedge());
    for (size_t i = 0; i < hlist.size(); i++) {
        Vec3 dir = f2->center() - vpos[i];
        float len = dir.norm();
        hlist[i]->vertex()->pos = vpos[i] + 1.0f/3.0f * len * dir.normalize();
    }
    return f2;
}

std::optional<Halfedge_Mesh::VertexRef> Halfedge_Mesh::insert_vertex(Halfedge_Mesh::FaceRef f) {
    HalfedgeRef h = f->halfedge();
    std::vector<HalfedgeRef> hlist;
    std::vector<VertexRef> vlist;
    Vec3 cpos (0,0,0);
    HalfedgeRef start = h;
    do {
        hlist.push_back(start);
        cpos += start->vertex()->pos;
        vlist.push_back(start->vertex());
        start = start->next();
    } while (start != f->halfedge());
    cpos *= 1.0f/vlist.size();
    VertexRef c = new_vertex();
    c->pos = cpos;
    EdgeRef e = new_edge();
    HalfedgeRef h0 = new_halfedge();
    HalfedgeRef h0t = new_halfedge();
    h0->twin() = h0t;
    h0t->twin() = h0;
    e->halfedge() = h0;
    h0->edge() = e;
    h0t->edge() = e;
    h0->face() = h->face();
    h0t->face() = h->face();
    h0->vertex() = vlist[0];
    h0t->vertex() = c;
    c->halfedge() = h0t;
    hlist[hlist.size()-1]->next() = h0;
    h0->next() = h0t;
    h0t->next() = hlist[0];
    HalfedgeRef back = h0t;
    for (size_t i = 1; i < hlist.size(); i++) {
        FaceRef newf = new_face();
        EdgeRef ep = new_edge();
        HalfedgeRef h1 = new_halfedge();
        HalfedgeRef h1t = new_halfedge();
        h1->twin() = h1t;
        h1t->twin() = h1;
        h1->edge() = ep;
        h1t->edge() = ep;
        ep->halfedge() = h1t;
        h1->face() = newf;
        h1t->face() = hlist[i]->face();
        h1->vertex() = hlist[i]->vertex();
        h1t->vertex() = c;
        h1->next() = back;
        back->face() = newf;
        back->next() = hlist[i-1];
        hlist[i-1]->face() = newf;
        hlist[i-1]->next() = h1;
        newf->halfedge() = back;
        back = h1t;
    }
    h0->next() = back;
    back->next() = hlist[hlist.size()-1];
    hlist[hlist.size()-1]->next() = h0;
    h0->face() = f;
    back->face() = f;
    hlist[hlist.size()-1]->face() = f;
    f->halfedge() = hlist[hlist.size()-1];
    return c;
}

/*
    This method should replace the face f with an additional, inset face
    (and ring of faces around it), corresponding to a bevel operation. It
    should return the new face.  NOTE: This method is responsible for updating
    the *connectivity* of the mesh only---it does not need to update the vertex
    positions. These positions will be updated in
    Halfedge_Mesh::bevel_face_positions (which you also have to
    implement!)
*/
std::optional<Halfedge_Mesh::FaceRef> Halfedge_Mesh::bevel_face(Halfedge_Mesh::FaceRef f) {

    // Reminder: You should set the positions of new vertices (v->pos) to be exactly
    // the same as wherever they "started from."

    //(void)f;
    //return std::nullopt;
    /*if(f->is_boundary()) {
        return f;
    }*/
    HalfedgeRef h = f->halfedge();
    VertexRef v = h->vertex();
    //Vec3 mid = v->pos;
    std::vector<HalfedgeRef> hlist;
    std::vector<VertexRef> vlist;
    std::vector<VertexRef> newvlist;
    std::vector<HalfedgeRef> newhlist;
    std::vector<HalfedgeRef> newhtlist;
    std::vector<HalfedgeRef> midhlist;
    FaceRef newF = new_face();
    int len = 1;
    hlist.push_back(h);
    HalfedgeRef start = h;
    vlist.push_back(v);
    do {
        start = start->next();
        hlist.push_back(start);
        vlist.push_back(start->vertex());
        //mid += start->vertex()->pos;
        len++;
    } while (start->next() != h);
    //mid *= 1.0f/len;
    for(int i = 0; i < len; i++) {
        VertexRef currv = vlist[i];
        HalfedgeRef currh = hlist[i];
        VertexRef newv = new_vertex();
        //newv->pos = (currv->pos + mid)/2;
        newv->pos = currv->pos;
        HalfedgeRef newh = new_halfedge();
        HalfedgeRef newht = new_halfedge();
        EdgeRef newe = new_edge();
        newh->twin() = newht;
        newht->twin() = newh;
        newh->face() = f;
        newht->face() = f;
        newh->edge() = newe;
        newht->edge() = newe;
        newe->halfedge() = newh;
        newv->halfedge() = newht;
        int ind = (i == 0)? len-1 : (i-1);
        newh->vertex() = currv;
        newht->vertex() = newv;
        hlist[ind]->next() = newh;
        newht->next() = currh;
        newvlist.push_back(newv);
        newhlist.push_back(newh);
        newhtlist.push_back(newht);
    }
    for(int i = 0; i < len; i++) {
        EdgeRef newe = new_edge();
        HalfedgeRef newh = new_halfedge();
        HalfedgeRef newht = new_halfedge();
        FaceRef newf = new_face();
        int indplus = (i+1)%len;
        newh->twin() = newht;
        newht->twin() = newh;
        newh->edge() = newe;
        newht->edge() = newe;
        newe->halfedge() = newh;
        newf->halfedge() = newh;
        newh->vertex() = newvlist[indplus];
        newht->vertex() = newvlist[i];
        newh->face() = newf;
        newht->face() = newF;
        newh->next() = newhtlist[i];
        newhtlist[i]->face() = newf;
        newhtlist[i]->next()->face() = newf;
        newhtlist[i]->next()->next() = newhlist[indplus];
        newhlist[indplus]->next() = newh;
        //newhlist[indplus]->next()->face() = newf;
        newhlist[indplus]->face() = newf;
        midhlist.push_back(newht);
    }
    for (int i = 0; i < len; i++) {
        midhlist[i]->next() = midhlist[(i+1)%len];
        midhlist[i]->face() = newF;
    }
    newF->halfedge() = midhlist[0];
    erase(f);
    return newF;
    
}

/*
    Compute new vertex positions for the vertices of the beveled vertex.

    These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 1, ..., new_halfedges.size()-1.

    The basic strategy here is to loop over the list of outgoing halfedges,
    and use the original vertex position and its associated outgoing edge
    to compute a new vertex position along the outgoing edge.
*/
void Halfedge_Mesh::bevel_vertex_positions(const std::vector<Vec3>& start_positions,
                                           Halfedge_Mesh::FaceRef face, float tangent_offset) {

    std::vector<HalfedgeRef> new_halfedges;
    auto h = face->halfedge();
    do {
        new_halfedges.push_back(h);
        h = h->next();
    } while(h != face->halfedge());

    /*(void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;*/
    std::vector<Vec3> tangents; 
    //Vec3 orig = face->center();
    for(size_t i = 0; i < new_halfedges.size(); i++) {
        Vec3 v0 = new_halfedges[i]->twin()->next()->twin()->vertex()->pos;
        Vec3 dir = start_positions[i]-v0;
        tangents.push_back(dir.normalize());
    }

    for(size_t i = 0; i < new_halfedges.size(); i++) {
        new_halfedges[i]->vertex()->pos = start_positions[i] + (tangent_offset * tangents[i]);
    }

}
/*
    Compute new vertex positions for the vertices of the extruded vertex.
*/

void Halfedge_Mesh::extrude_vertex_positions(const Vec3 start_positions,
                                           Halfedge_Mesh::FaceRef face) {

    std::vector<HalfedgeRef> new_halfedges;
    auto h = face->halfedge();
    do {
        new_halfedges.push_back(h);
        h = h->next();
    } while(h != face->halfedge());

    /*(void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;*/
    std::vector<Vec3> tangents;
    float tanLen = 0.0f; 
    //Vec3 orig = face->center();
    for(size_t i = 0; i < new_halfedges.size(); i++) {
        Vec3 v0 = new_halfedges[i]->twin()->next()->twin()->vertex()->pos;
        Vec3 dir = v0-start_positions;
        tanLen += dir.norm();
        tangents.push_back(dir.normalize());
    }
    tanLen *= 0.3f/tangents.size();

    for(size_t i = 0; i < new_halfedges.size(); i++) {
        new_halfedges[i]->vertex()->pos = start_positions + (tanLen * tangents[i]);
    }

}

void Halfedge_Mesh::extrude_vertex_pos(const std::vector<Vec3>& start_positions, Halfedge_Mesh::VertexRef v, float normal_offset) {
    v->pos = start_positions[0] + v->normal() * normal_offset;
}

/*
    Compute new vertex positions for the vertices of the beveled edge.

    These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 1, ..., new_halfedges.size()-1.

    The basic strategy here is to loop over the list of outgoing halfedges,
    and use the preceding and next vertex position from the original mesh
    (in the orig array) to compute an offset vertex position.

    Note that there is a 1-to-1 correspondence between halfedges in
    newHalfedges and vertex positions
    in orig.  So, you can write loops of the form

    for(size_t i = 0; i < new_halfedges.size(); i++)
    {
            Vector3D pi = start_positions[i]; // get the original vertex
            position corresponding to vertex i
    }
*/
void Halfedge_Mesh::bevel_edge_positions(const std::vector<Vec3>& start_positions,
                                         Halfedge_Mesh::FaceRef face, float tangent_offset) {

    std::vector<HalfedgeRef> new_halfedges;
    auto h = face->halfedge();
    do {
        new_halfedges.push_back(h);
        h = h->next();
    } while(h != face->halfedge());

    /*(void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;*/
    std::vector<Vec3> tangents;
    for(size_t i = 0; i < new_halfedges.size(); i++) {
        Vec3 v0 = new_halfedges[i]->twin()->next()->twin()->vertex()->pos;
        Vec3 dir = start_positions[i]-v0;
        tangents.push_back(dir);
    }
    for(size_t i = 0; i < new_halfedges.size(); i++) {
        new_halfedges[i]->vertex()->pos = start_positions[i] + (tangent_offset * tangents[i]);
    }
    
}

/*
    Compute new vertex positions for the vertices of the beveled face.

    These vertices can be accessed via new_halfedges[i]->vertex()->pos for
    i = 1, ..., new_halfedges.size()-1.

    The basic strategy here is to loop over the list of outgoing halfedges,
    and use the preceding and next vertex position from the original mesh
    (in the start_positions array) to compute an offset vertex
    position.

    Note that there is a 1-to-1 correspondence between halfedges in
    new_halfedges and vertex positions
    in orig. So, you can write loops of the form

    for(size_t i = 0; i < new_halfedges.size(); i++)
    {
            Vec3 pi = start_positions[i]; // get the original vertex
            position corresponding to vertex i
    }
*/
void Halfedge_Mesh::bevel_face_positions(const std::vector<Vec3>& start_positions,
                                         Halfedge_Mesh::FaceRef face, float tangent_offset,
                                         float normal_offset) {

    if(flip_orientation) normal_offset = -normal_offset;
    std::vector<HalfedgeRef> new_halfedges;
    auto h = face->halfedge();
    do {
        new_halfedges.push_back(h);
        h = h->next();
    } while(h != face->halfedge());

    /*(void)new_halfedges;
    (void)start_positions;
    (void)face;
    (void)tangent_offset;
    (void)normal_offset;*/
    size_t len = new_halfedges.size();
    
    for(size_t i = 0; i < len; i++) {
        Vec3 tangent = face->center() - start_positions[i];
        Vec3 normal = -face->normal();
        new_halfedges[i]->vertex()->pos = start_positions[i] + (normal_offset*normal + tangent_offset*tangent);
    }
}

/*
    Splits all non-triangular faces into triangles.
*/
void Halfedge_Mesh::triangulate() {

    // For each face...
    //int num = 0;
    for(FaceRef f = faces_begin(); f != faces_end(); f++) {
        
                
        if(f->is_boundary()) continue;
        std::vector<HalfedgeRef> hlist;
        HalfedgeRef start = f->halfedge();
        do {
            hlist.push_back(start);
            start = start->next();
        } while (start != f->halfedge());
        size_t deg = hlist.size();
        if(deg == 3) continue;
        

        HalfedgeRef h = f->halfedge();
        HalfedgeRef hp = hlist[deg-1];
        HalfedgeRef hn = hlist[1];
        HalfedgeRef hnn = hlist[2];
        EdgeRef e0 = new_edge();
        HalfedgeRef h0 = new_halfedge();
        HalfedgeRef h0t = new_halfedge();
        FaceRef f0 = new_face();
        h0->twin() = h0t;
        h0t->twin() = h0;
        h0->edge() = e0;
        h0t->edge() = e0;
        e0->halfedge() = h0;
        h0->vertex() = hn->twin()->vertex();
        h0t->vertex() = h->vertex();
        h->next() = hn;
        hn->next() = h0;
        h0->next() = h;
        hp->next() = h0t;
        h0t->next() = hnn;
        h0t->face() = hp->face();
        hp->face()->halfedge() = hp;
        h->face() = f0;
        hn->face() = f0;
        h0->face() = f0;
        f0->halfedge() = h0;
        HalfedgeRef curr = h0t;
        HalfedgeRef nex = hnn;
        HalfedgeRef pre = hp;

        for(size_t i = 1; i < deg-3; i++) {
            if ((deg-i)%2 == 0) {
                EdgeRef ei = new_edge();
                HalfedgeRef hi = new_halfedge();
                HalfedgeRef hit = new_halfedge();
                FaceRef fi = new_face();
                HalfedgeRef nex2 = nex->next();
                hi->twin() = hit;
                hit->twin() = hi;
                hi->edge() = ei;
                hit->edge() = ei;
                ei->halfedge() = hi;
                curr->next() = nex;
                nex->next() = hi;
                hi->next() = curr;
                curr->face() = fi;
                nex->face() = fi;
                hi->face() = fi;
                pre->next() = hit;
                hit->next() = nex2;
                hi->vertex() = nex2->vertex();
                hit->vertex() = curr->vertex();
                hit->face() = pre->face();
                pre->face()->halfedge() = pre;
                fi->halfedge() = hi;
                curr = hit;
                nex = nex2;
            } else {
                EdgeRef ei = new_edge();
                HalfedgeRef hi = new_halfedge();
                HalfedgeRef hit = new_halfedge();
                FaceRef fi = new_face();
                HalfedgeRef hpp = pre->twin()->next()->twin();
                hi->twin() = hit;
                hit->twin() = hi;
                hi->edge() = ei;
                hit->edge() = ei;
                ei->halfedge() = hi;
                curr->next() = hi;
                hi->next() = pre;
                pre->next() = curr;
                curr->face() = fi;
                hi->face() = fi;
                pre->face() = fi;
                hpp->next() = hit;
                hit->next() = nex;
                fi->halfedge() = hi;
                hi->vertex() = nex->vertex();
                hit->vertex() = pre->vertex();
                hit->face() = hpp->face();
                hpp->face()->halfedge() = hpp;
                curr = hit;
                pre = hpp;
            }
        }

    }
}


/* Note on the quad subdivision process:

        Unlike the local mesh operations (like bevel or edge flip), we will perform
        subdivision by splitting *all* faces into quads "simultaneously."  Rather
        than operating directly on the halfedge data structure (which as you've
        seen is quite difficult to maintain!) we are going to do something a bit nicer:
           1. Create a raw list of vertex positions and faces (rather than a full-
              blown halfedge mesh).
           2. Build a new halfedge mesh from these lists, replacing the old one.
        Sometimes rebuilding a data structure from scratch is simpler (and even
        more efficient) than incrementally modifying the existing one.  These steps are
        detailed below.

  Step I: Compute the vertex positions for the subdivided mesh.
        Here we're going to do something a little bit strange: since we will
        have one vertex in the subdivided mesh for each vertex, edge, and face in
        the original mesh, we can nicely store the new vertex *positions* as
        attributes on vertices, edges, and faces of the original mesh. These positions
        can then be conveniently copied into the new, subdivided mesh.
        This is what you will implement in linear_subdivide_positions() and
        catmullclark_subdivide_positions().

  Steps II-IV are provided (see Halfedge_Mesh::subdivide()), but are still detailed
  here:

  Step II: Assign a unique index (starting at 0) to each vertex, edge, and
        face in the original mesh. These indices will be the indices of the
        vertices in the new (subdivided mesh).  They do not have to be assigned
        in any particular order, so long as no index is shared by more than one
        mesh element, and the total number of indices is equal to V+E+F, i.e.,
        the total number of vertices plus edges plus faces in the original mesh.
        Basically we just need a one-to-one mapping between original mesh elements
        and subdivided mesh vertices.

  Step III: Build a list of quads in the new (subdivided) mesh, as tuples of
        the element indices defined above. In other words, each new quad should be
        of the form (i,j,k,l), where i,j,k and l are four of the indices stored on
        our original mesh elements.  Note that it is essential to get the orientation
        right here: (i,j,k,l) is not the same as (l,k,j,i).  Indices of new faces
        should circulate in the same direction as old faces (think about the right-hand
        rule).

  Step IV: Pass the list of vertices and quads to a routine that clears
        the internal data for this halfedge mesh, and builds new halfedge data from
        scratch, using the two lists.
*/

/*
    Compute new vertex positions for a mesh that splits each polygon
    into quads (by inserting a vertex at the face midpoint and each
    of the edge midpoints).  The new vertex positions will be stored
    in the members Vertex::new_pos, Edge::new_pos, and
    Face::new_pos.  The values of the positions are based on
    simple linear interpolation, e.g., the edge midpoints and face
    centroids.
*/
void Halfedge_Mesh::linear_subdivide_positions() {

    // For each vertex, assign Vertex::new_pos to
    // its original position, Vertex::pos.
    for (VertexRef v = vertices_begin(); v != vertices_end(); v++) {
        v->new_pos = v->pos;
    }

    // For each edge, assign the midpoint of the two original
    // positions to Edge::new_pos.
    for (EdgeRef e = edges_begin(); e != edges_end(); e++) {
        Vec3 cent = e->halfedge()->vertex()->pos + e->halfedge()->twin()->vertex()->pos;
        cent /= 2.0f;
        e->new_pos = cent;
    }

    // For each face, assign the centroid (i.e., arithmetic mean)
    // of the original vertex positions to Face::new_pos. Note
    // that in general, NOT all faces will be triangles!
    for (FaceRef f = faces_begin(); f != faces_end(); f++) {
        Vec3 cent(0,0,0);
        int n = 0;
        HalfedgeRef h = f->halfedge();
        do {
            cent += h->vertex()->pos;
            n++;
            h = h->next();
        } while (h != f->halfedge());
        cent *= 1.0f/n;
        f->new_pos = cent;
    }
}

/*
    Compute new vertex positions for a mesh that splits each polygon
    into quads (by inserting a vertex at the face midpoint and each
    of the edge midpoints).  The new vertex positions will be stored
    in the members Vertex::new_pos, Edge::new_pos, and
    Face::new_pos.  The values of the positions are based on
    the Catmull-Clark rules for subdivision.

    Note: this will only be called on meshes without boundary
*/
void Halfedge_Mesh::catmullclark_subdivide_positions() {

    // The implementation for this routine should be
    // a lot like Halfedge_Mesh:linear_subdivide_positions:(),
    // except that the calculation of the positions themsevles is
    // slightly more involved, using the Catmull-Clark subdivision
    // rules. (These rules are outlined in the Developer Manual.)

    // Faces
    for (FaceRef f = faces_begin(); f != faces_end(); f++) {
        Vec3 cent(0,0,0);
        int n = 0;
        HalfedgeRef h = f->halfedge();
        do {
            cent += h->vertex()->pos;
            n++;
            h = h->next();
        } while (h != f->halfedge());
        cent *= 1.0f/n;
        f->new_pos = cent;
    }

    // Edges
    for (EdgeRef e = edges_begin(); e != edges_end(); e++) {
        Vec3 cent = e->halfedge()->vertex()->pos + e->halfedge()->twin()->vertex()->pos;
        cent += e->halfedge()->face()->new_pos + e->halfedge()->twin()->face()->new_pos;
        cent /= 4.0f;
        e->new_pos = cent;
    }

    // Vertices
    for (VertexRef v = vertices_begin(); v != vertices_end(); v++) {
        Vec3 Q(0,0,0), R(0,0,0);
        HalfedgeRef h = v->halfedge();
        float deg = 0.0f;
        do {
            Q += h->face()->new_pos;
            R += h->edge()->new_pos;
            deg++;
            h = h->twin()->next();
        } while (h != v->halfedge());
        Q *= 1.0f/deg;
        R *= 1.0f/deg;
        v->new_pos = (Q + 2*R + (deg-3)*(v->pos))/deg;
    }
}

/*
        This routine should increase the number of triangles in the mesh
        using Loop subdivision. Note: this is will only be called on triangle meshes.
*/
void Halfedge_Mesh::loop_subdivide() {

    // Compute new positions for all the vertices in the input mesh, using
    // the Loop subdivision rule, and store them in Vertex::new_pos.
    // -> At this point, we also want to mark each vertex as being a vertex of the
    //    original mesh. Use Vertex::is_new for this.
    // -> Next, compute the updated vertex positions associated with edges, and
    //    store it in Edge::new_pos.
    // -> Next, we're going to split every edge in the mesh, in any order.  For
    //    future reference, we're also going to store some information about which
    //    subdivided edges come from splitting an edge in the original mesh, and
    //    which edges are new, by setting the flat Edge::is_new. Note that in this
    //    loop, we only want to iterate over edges of the original mesh.
    //    Otherwise, we'll end up splitting edges that we just split (and the
    //    loop will never end!)
    // -> Now flip any new edge that connects an old and new vertex.
    // -> Finally, copy the new vertex positions into final Vertex::pos.
    for (VertexRef v = vertices_begin(); v != vertices_end(); v++) {
        v->is_new = false;
    }
    

    // Each vertex and edge of the original surface can be associated with a
    // vertex in the new (subdivided) surface.
    // Therefore, our strategy for computing the subdivided vertex locations is to
    // *first* compute the new positions
    // using the connectivity of the original (coarse) mesh; navigating this mesh
    // will be much easier than navigating
    // the new subdivided (fine) mesh, which has more elements to traverse.  We
    // will then assign vertex positions in
    // the new mesh based on the values we computed for the original mesh.

    // Compute updated positions for all the vertices in the original mesh, using
    // the Loop subdivision rule.
    for (VertexRef v = vertices_begin(); v != vertices_end(); v++) {
        int n = 0;
        Vec3 orig = v->pos;
        
        Vec3 sum(0,0,0);
        HalfedgeRef start = v->halfedge();
        do {
            sum += start->twin()->vertex()->pos;
            n++;
            start = start->twin()->next();
        } while (start != v->halfedge());
        float u = (n == 3) ? 3.0f/16.0f : 3.0f/(8*n);
        v->new_pos = (1.0f-n*u) * orig + u * sum;
        //v->new_pos = v->pos;
    }

    // Next, compute the updated vertex positions associated with edges.
    for (EdgeRef e = edges_begin(); e != edges_end(); e++) {
        e->is_new = false;
        Vec3 A = e->halfedge()->vertex()->pos;
        Vec3 B = e->halfedge()->twin()->vertex()->pos;
        Vec3 C = e->halfedge()->next()->next()->vertex()->pos;
        Vec3 D = e->halfedge()->twin()->next()->next()->vertex()->pos;
        e->new_pos = 3.0f/8.0f * (A + B) + 1.0f/8.0f * (C + D);
    }

    // Next, we're going to split every edge in the mesh, in any order. For
    // future reference, we're also going to store some information about which
    // subdivided edges come from splitting an edge in the original mesh, and
    // which edges are new.
    // In this loop, we only want to iterate over edges of the original
    // mesh---otherwise, we'll end up splitting edges that we just split (and
    // the loop will never end!)
    EdgeRef e0 = edges_begin();
    size_t n = n_edges();
    for (size_t i = 0; i < n; i++) {
        EdgeRef next = e0;
        next++;
        if (!e0->is_new) {
            auto ref = split_edge(e0);
            if(!ref.has_value()) {
                continue;
            }
            VertexRef v = ref.value();
            v->new_pos = e0->new_pos;
            v->is_new = true;
            
        }
        e0 = next;
    }

    // Finally, flip any new edge that connects an old and new vertex.
    for (EdgeRef e = edges_begin(); e != edges_end(); e++) {
        if (!e->is_new) continue;
        VertexRef v0 = e->halfedge()->vertex();
        VertexRef v1 = e->halfedge()->twin()->vertex();
        if (v0->is_new != v1->is_new) {
            flip_edge(e);
        }
    }

    // Copy the updated vertex positions to the subdivided mesh.
    for (VertexRef v = vertices_begin(); v != vertices_end(); v++) {
        v->pos = v->new_pos;
    }
}

/*
    Isotropic remeshing. Note that this function returns success in a similar
    manner to the local operations, except with only a boolean value.
    (e.g. you may want to return false if this is not a triangle mesh)
*/
bool Halfedge_Mesh::isotropic_remesh() {
    
    // Compute the mean edge length.
    // Repeat the four main steps for 5 or 6 iterations
    // -> Split edges much longer than the target length (being careful about
    //    how the loop is written!)
    // -> Collapse edges much shorter than the target length.  Here we need to
    //    be EXTRA careful about advancing the loop, because many edges may have
    //    been destroyed by a collapse (which ones?)
    // -> Now flip each edge if it improves vertex degree
    // -> Finally, apply some tangential smoothing to the vertex positions
    /*for (FaceRef f = faces_begin(); f != faces_end(); f++) {
        if (f->degree() != 3) return false;
    }*/
    triangulate();
    float mean = 0.0f;
    for (EdgeRef e = edges_begin(); e != edges_end(); e++) {
            mean += e->length();
            //e->is_new = true;
    }
    mean *= 1.0f/n_edges();
    for(int iter = 0; iter < 5; iter++) {
        size_t n = n_edges();
        EdgeRef e = edges_begin();
        for (size_t i = 0; i < n; i++) {
            EdgeRef next = e;
            next++;
            float elen = e->length();
            //float elen = (e->halfedge()->vertex()->pos - e->halfedge()->twin()->vertex()->pos).norm();
            if (elen > 4*mean/3) {
                split_edge(e);
                if(validate().has_value()) {
                    std::cerr << validate().value().second << '\n';
                }
            }
            e = next;
        }
        std::cerr << "Iteration number " << iter << std::endl;
        EdgeRef e2 = edges_begin();
        while (e2 != edges_end()) {
            if(e2 == edges_end()) break;
            
            EdgeRef next = e2;
            next++;
            VertexRef v1 = e2->halfedge()->vertex();
            if(isnan(v1->pos.x) || isnan(v1->pos.y) || isnan(v1->pos.z)) {
                std::cout << "e2 has nan: " << e2->id() << std::endl;
                e2 = next;
                continue;
            }
            float elen = e2->length();
            //float elen = (e2->halfedge()->vertex()->pos - e2->halfedge()->twin()->vertex()->pos).norm();
            if (elen < 0.8*mean) {
                EdgeRef A = e2->halfedge()->next()->next()->edge();
                EdgeRef B = e2->halfedge()->twin()->next()->next()->edge();
                EdgeRef C = e2->halfedge()->next()->edge();
                EdgeRef D = e2->halfedge()->twin()->next()->edge();
                std::cerr<< "A: " <<A->id() << std::endl;
                std::cerr<< "B: "<< B->id() << std::endl;
                std::cerr<< "C: "<< C->id() << std::endl;
                std::cerr<< "D: "<< D->id() << std::endl;
                while (next == A || next == B || next == C || next == D) {
                    next++;
                }
                std::cerr << "Collapsing edge: " << e2->id() << std::endl;
                std::cerr << "Next edge to collapse: " << next->id() << std::endl;
                if(next == edges_end()) std::cerr << "Next edge is null" << std::endl;
                auto ref = collapse_edge_erase(e2);
                if(!ref.has_value()) {
                    std::cerr << "collapse aborted" << std::endl;
                    VertexRef vx = e2->halfedge()->vertex();
                    if(isnan(vx->pos.x) || isnan(vx->pos.y) || isnan(vx->pos.z)) {
                        std::cout << "e2 has nan but again: " << e2->id() << std::endl;
                    }
                }
                //std::cerr << "e2 id " << e2->id() << std::endl;
                /*auto ref = validate();
                if(ref.has_value()) {
                    std::cerr << id_of(ref.value().first) << "; " << ref.value().second << '\n';
                }*/
                
            }
            if(next == edges_end()) {
                break;
            }
            
            e2 = next;
            
        }
        for (EdgeRef e0 = edges_begin(); e0 != edges_end(); e0++) {
            HalfedgeRef h = e0->halfedge();
            HalfedgeRef ht = e0->halfedge()->twin();
            int dev0 = abs((int)h->vertex()->degree()-6) + abs((int)ht->vertex()->degree()-6) + 
                abs((int)h->next()->next()->vertex()->degree()-6) + abs((int)ht->next()->next()->vertex()->degree()-6);
            int dev1 = abs((int)h->vertex()->degree()-7) + abs((int)ht->vertex()->degree()-7) + 
                abs((int)h->next()->next()->vertex()->degree()-5) + abs((int)ht->next()->next()->vertex()->degree()-5);
            if (dev0 > dev1) {
                flip_edge(e0);
                if(validate().has_value()) {
                    std::cerr << validate().value().second << '\n';
                }
            }
        }

        for(VertexRef v = vertices_begin(); v != vertices_end(); v++) {
            Vec3 orig = v->pos;
            Vec3 v0 = 0.2 * (v->neighborhood_center() - orig) + orig;
            Vec3 v1 = v0 - dot(v->normal(), v0)*(v->normal());
            v->new_pos = v1;
        }
        for(VertexRef v = vertices_begin(); v != vertices_end(); v++) {
            v->pos = v->new_pos;
        }
    }
    return true;

    // Note: if you erase elements in a local operation, they will not be actually deleted
    // until do_erase or validate are called. This is to facilitate checking
    // for dangling references to elements that will be erased.
    // The rest of the codebase will automatically call validate() after each op,
    // but here simply calling collapse_edge() will not erase the elements.
    // You should use collapse_edge_erase() instead for the desired behavior.

    //return false;
}

/* Helper type for quadric simplification */
struct Edge_Record {
    Edge_Record() {
    }
    Edge_Record(std::unordered_map<Halfedge_Mesh::VertexRef, Mat4>& vertex_quadrics,
                Halfedge_Mesh::EdgeRef e)
        : edge(e) {

        // Compute the combined quadric from the edge endpoints.
        // -> Build the 3x3 linear system whose solution minimizes the quadric error
        //    associated with these two endpoints.
        // -> Use this system to solve for the optimal position, and store it in
        //    Edge_Record::optimal.
        // -> Also store the cost associated with collapsing this edge in
        //    Edge_Record::cost.
        auto k1 = vertex_quadrics.find(e->halfedge()->vertex());
        auto k2 = vertex_quadrics.find(e->halfedge()->twin()->vertex());
        Mat4 K = Mat4::Zero;
        if (k1 == vertex_quadrics.end() || k2 == vertex_quadrics.end()) {
            //TODO
        } else {
            K = k1->second + k2->second;
        }
        Mat4 A = Mat4::I;
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                A[i][j] = K[i][j];
            }
        }
        Vec3 b (-K[0][3], -K[1][3], -K[2][3]);
        this->optimal = A.inverse() * b;
        Vec4 v (this->optimal, 1.0);
        this->cost = dot(v, K*v);
        
    }
    Halfedge_Mesh::EdgeRef edge;
    Vec3 optimal;
    float cost;
};

/* Comparison operator for Edge_Records so std::set will properly order them */
bool operator<(const Edge_Record& r1, const Edge_Record& r2) {
    if(r1.cost != r2.cost) {
        return r1.cost < r2.cost;
    }
    Halfedge_Mesh::EdgeRef e1 = r1.edge;
    Halfedge_Mesh::EdgeRef e2 = r2.edge;
    return &*e1 < &*e2;
}

/** Helper type for quadric simplification
 *
 * A PQueue is a minimum-priority queue that
 * allows elements to be both inserted and removed from the
 * queue.  Together, one can easily change the priority of
 * an item by removing it, and re-inserting the same item
 * but with a different priority.  A priority queue, for
 * those who don't remember or haven't seen it before, is a
 * data structure that always keeps track of the item with
 * the smallest priority or "score," even as new elements
 * are inserted and removed.  Priority queues are often an
 * essential component of greedy algorithms, where one wants
 * to iteratively operate on the current "best" element.
 *
 * PQueue is templated on the type T of the object
 * being queued.  For this reason, T must define a comparison
 * operator of the form
 *
 *    bool operator<( const T& t1, const T& t2 )
 *
 * which returns true if and only if t1 is considered to have a
 * lower priority than t2.
 *
 * Basic use of a PQueue might look
 * something like this:
 *
 *    // initialize an empty queue
 *    PQueue<myItemType> queue;
 *
 *    // add some items (which we assume have been created
 *    // elsewhere, each of which has its priority stored as
 *    // some kind of internal member variable)
 *    queue.insert( item1 );
 *    queue.insert( item2 );
 *    queue.insert( item3 );
 *
 *    // get the highest priority item currently in the queue
 *    myItemType highestPriorityItem = queue.top();
 *
 *    // remove the highest priority item, automatically
 *    // promoting the next-highest priority item to the top
 *    queue.pop();
 *
 *    myItemType nextHighestPriorityItem = queue.top();
 *
 *    // Etc.
 *
 *    // We can also remove an item, making sure it is no
 *    // longer in the queue (note that this item may already
 *    // have been removed, if it was the 1st or 2nd-highest
 *    // priority item!)
 *    queue.remove( item2 );
 *
 */
template<class T> struct PQueue {
    void insert(const T& item) {
        queue.insert(item);
    }
    void remove(const T& item) {
        if(queue.find(item) != queue.end()) {
            queue.erase(item);
        }
    }
    const T& top(void) const {
        return *(queue.begin());
    }
    void pop(void) {
        queue.erase(queue.begin());
    }
    size_t size() {
        return queue.size();
    }

    std::set<T> queue;
};

/*
    Mesh simplification. Note that this function returns success in a similar
    manner to the local operations, except with only a boolean value.
    (e.g. you may want to return false if you can't simplify the mesh any
    further without destroying it.)
*/
bool Halfedge_Mesh::simplify() {

    std::unordered_map<VertexRef, Mat4> vertex_quadrics;
    std::unordered_map<FaceRef, Mat4> face_quadrics;
    std::unordered_map<EdgeRef, Edge_Record> edge_records;
    PQueue<Edge_Record> edge_queue;

    // Compute initial quadrics for each face by simply writing the plane equation
    // for the face in homogeneous coordinates. These quadrics should be stored
    // in face_quadrics
    // -> Compute an initial quadric for each vertex as the sum of the quadrics
    //    associated with the incident faces, storing it in vertex_quadrics
    // -> Build a priority queue of edges according to their quadric error cost,
    //    i.e., by building an Edge_Record for each edge and sticking it in the
    //    queue. You may want to use the above PQueue<Edge_Record> for this.
    // -> Until we reach the target edge budget, collapse the best edge. Remember
    //    to remove from the queue any edge that touches the collapsing edge
    //    BEFORE it gets collapsed, and add back into the queue any edge touching
    //    the collapsed vertex AFTER it's been collapsed. Also remember to assign
    //    a quadric to the collapsed vertex, and to pop the collapsed edge off the
    //    top of the queue.
    for (FaceRef f = faces_begin(); f != faces_end(); f++) {
        Mat4 quad = Mat4::Zero;
        Vec3 n = f->normal();
        float d = -dot(n, f->halfedge()->vertex()->pos);
        Vec4 v (n, d);
        face_quadrics[f] = outer(v, v);
    }

    for (VertexRef v = vertices_begin(); v != vertices_end(); v++) {
        Mat4 quad = Mat4::Zero;
        HalfedgeRef start = v->halfedge();
        do {
            auto ref = face_quadrics.find(start->face());
            if(ref != face_quadrics.end()) {
                quad += ref->second;
            }
            start = start->twin()->next();
        } while (start != v->halfedge());
        vertex_quadrics[v] = quad;
    }
    for (EdgeRef e = edges_begin(); e != edges_end(); e++) {
        Edge_Record rec(vertex_quadrics, e);
        edge_records[e] = rec;
        edge_queue.insert(rec);
    }
    size_t target = n_faces()/4;
    while (n_faces() > target) {
        Edge_Record er = edge_queue.top();
        EdgeRef e = er.edge;
        edge_queue.pop();
        HalfedgeRef h = e->halfedge()->twin()->next();
        do {
            auto rec = edge_records.find(h->edge());
            if (rec != edge_records.end()) {
                Edge_Record er1 = rec->second;
                edge_queue.remove(er1);
            }
            h = h->twin()->next();
            
        } while (h != e->halfedge());
        h = e->halfedge()->next();
        do {
            auto rec = edge_records.find(h->edge());
            if (rec != edge_records.end()) {
                Edge_Record er1 = rec->second;
                edge_queue.remove(er1);
            }
            h = h->twin()->next();
        } while (h != e->halfedge()->twin());
        auto vr = collapse_edge_erase(e);
        if (!vr.has_value()) {
            return false;
        }
        VertexRef v = vr.value();
        v->pos = er.optimal;
        Mat4 vquad = Mat4::Zero;
        HalfedgeRef h1 = v->halfedge();
        do {
            FaceRef f1 = h1->face();
            float d = -dot(f1->normal(), f1->halfedge()->vertex()->pos);
            Vec4 vnew (f1->normal(), d);
            face_quadrics[f1] = outer(vnew, vnew);
            vquad += outer(vnew, vnew);
            h1 = h1->twin()->next();
        } while (h1 != v->halfedge());
        h1 = v->halfedge();
        do {
            VertexRef vp = h1->twin()->vertex();
            Mat4 vpquad = Mat4::Zero;
            HalfedgeRef hvp = vp->halfedge();
            do {
                FaceRef f1 = hvp->face();
                float d = -dot(f1->normal(), f1->halfedge()->vertex()->pos);
                Vec4 vnewp (f1->normal(), d);
                face_quadrics[f1] = outer(vnewp, vnewp);
                vpquad += outer(vnewp, vnewp);
                hvp = hvp->twin()->next();
            } while (hvp != vp->halfedge());
            vertex_quadrics[vp] = vpquad;
            h1 = h1->twin()->next();
            
        } while (h1 != v->halfedge());
        vertex_quadrics[v] = vquad;
        h1 = v->halfedge();
        do {
            EdgeRef ep = h1->edge();
            Edge_Record recp(vertex_quadrics, ep);
            edge_records[ep] = recp;
            edge_queue.insert(recp);
            h1 = h1->twin()->next();
        } while (h1 != v->halfedge());

    }
    return true;
    

    // Note: if you erase elements in a local operation, they will not be actually deleted
    // until do_erase or validate are called. This is to facilitate checking
    // for dangling references to elements that will be erased.
    // The rest of the codebase will automatically call validate() after each op,
    // but here simply calling collapse_edge() will not erase the elements.
    // You should use collapse_edge_erase() instead for the desired behavior.

    //return false;
}
