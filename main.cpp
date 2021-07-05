
#include <iostream>
#include <vector>
#include <queue>
#include <cstdlib> 
#include <stdio.h>
#include <string.h>

#define NULL nullptr
#define Espacio 1000
#define derecha 1
#define izquierda 0

#include "EasyBMP.hpp"
#include "TreeToImg.hpp"

#include <chrono>
using namespace std::chrono;


TreeToImg* tImg = new TreeToImg();

//ESTRUCTURA DE LA CAJA DIMENSIONADA CON RESPECTO A LAS DIMENSIONES DEL OBJETO CONTENIDO
typedef struct AABB {
    //PUNTOS DE LA CAJA
    int minX, maxX, minY, maxY;

    //CONSTRUCTORES
    AABB() {};
    //AABB(int min_X, int min_Y, int max_X, int max_Y) : minX(min_X), maxX(max_X), minY(min_Y), maxY(max_Y) {}
    AABB(int x1, int y1, int x2, int y2) : minX(std::min(x1, x2)), minY(std::min(y1, y2)), maxX(std::max(x1, x2)), maxY(std::max(y1, y2)) {}
    
    float Area()
    {
        float length_X = maxX - minX;
        float length_Y = maxY - minY;
        return abs(length_X * length_Y);
    }

    bool overlap(AABB anotherBox) {
        return  this->minX < anotherBox.maxX &&
                this->maxX > anotherBox.minX &&
                this->minY < anotherBox.maxY &&
                this->maxY > anotherBox.minY;
    }

    
} AABB;

//ESTRUCTURA DEL NODO CONSTITUYENTE DEL ARBOL Y SUS METODOS
typedef struct Node {
    AABB box;
    int objectIndex;
    Node* parentIndex;
    Node* leftChild;
    Node* rightChild;
    bool isLeaf;

    Node() : parentIndex(NULL), leftChild(NULL), rightChild(NULL), isLeaf(false) {}
    //Node(char c) : parentIndex(NULL), leftChild(NULL), rightChild(NULL), isLeaf(false) {
    // //  this->box = * new AABB((rand() % 1000 +1)/100.0f, rand()/100.0f, rand()/100.0f, rand()/100.0f);
    // };
    //CONSTRUCTOR PARA HOJAS    objectIndex
    Node(AABB box) : box(box), parentIndex(NULL), leftChild(NULL), rightChild(NULL), isLeaf(true) {} //objectIndex
    //CONSTRUCTOR PARA NODOS INTERMEDIOS
    //Node(int objectIndex) : parentIndex(NULL), leftChild(NULL), rightChild(NULL), isLeaf(false) {}
    
    AABB merge(const AABB anotherBox) {
        return AABB(
            std::min(this->box.minX, anotherBox.minX), std::min(this->box.minY, anotherBox.minY),
            std::max(this->box.maxX, anotherBox.maxX), std::max(this->box.maxY, anotherBox.maxY)
        );
    }

    AABB merge(const AABB box1, const AABB box2) {
        return AABB(
                std::min(box1.minX, box2.minX), std::min(box1.minY, box2.minY),
                std::max(box1.maxX, box2.maxX), std::max(box1.maxY, box2.maxY)
               );
    }

} Node;

//FUNCION DE COMPARACIÓN PARA EL COLA DE PRIORIDAD IMPLEMENTADA EN EL PickBestSibling
struct compare {
    bool operator()(Node* a, Node* b) {
        return a->box.Area() > b->box.Area();
    }
};

//ESTRUCTURA DEL ARBOL Y SUS METODOS
typedef struct Tree {
    std::vector<Node*> leafs;
    int leafCount;
    Node* rootIndex;

    //CONSTRUCTORES
    Tree() : leafCount(0), rootIndex(NULL) {}    

    //METODOS
    Node* AllocateInternalNode() {//objectIndex
        Node* node = new Node(); //objectIndex
        return node;
    }

    void createNewParent(Node* sibling, Node* leafIndex) { //objectIndex
        Node* oldParent = sibling->parentIndex;
        Node* newParentIndex = AllocateInternalNode(); //objectIndex
        newParentIndex->parentIndex = oldParent;
        newParentIndex->box = newParentIndex->merge(sibling->box, leafIndex->box);

        if (oldParent != NULL)
        {
            // The sibling was not the root
            if (oldParent->leftChild == sibling)
            {
                oldParent->leftChild = newParentIndex;
            }
            else
            {
                oldParent->rightChild = newParentIndex;
            }
            newParentIndex->leftChild = sibling;
            newParentIndex->rightChild = leafIndex;
            sibling->parentIndex = newParentIndex;
            leafIndex->parentIndex = newParentIndex;
        }
        else
        {
            // The sibling was the root
            newParentIndex->leftChild = sibling;
            newParentIndex->rightChild = leafIndex;
            sibling->parentIndex = newParentIndex;
            leafIndex->parentIndex = newParentIndex;
            this->rootIndex = newParentIndex;
        }

    }

    //CostBest = SA(newLeaf U sibling) + difSA(parent) +...+ difSA(root)
    float insertionCost(Node* indexSibling,AABB leafBox) {
        float cost = 0;
        Node* otherChild;
        Node* refitChildIndex = indexSibling;
        Node* parentIndex = indexSibling->parentIndex;
        AABB virtualBox = indexSibling->merge(leafBox);
        
        //se suma primero el area del virtualBox del padre tentativo en caso de inserción
        cost += virtualBox.Area();
        while (parentIndex != NULL) {
            if (parentIndex->leftChild == refitChildIndex)
                otherChild = parentIndex->rightChild;
            else otherChild = parentIndex->leftChild;

            virtualBox = otherChild->merge(virtualBox);
            cost += (virtualBox.Area() - parentIndex->box.Area());
            refitChildIndex = parentIndex;
            parentIndex = parentIndex->parentIndex;
            
        }

        return cost;
    }

    //CostLow = SA(newLeaf) + difSA(virtualBox) + difSA(parent) +...+ difSA(root)
    float explorationCost(Node* sibling, AABB leafBox) {
        float cost = 0;
        Node* otherChild;
        Node* refitChildIndex = sibling;
        Node* parentIndex = sibling->parentIndex;
        AABB virtualBox = sibling->merge(leafBox);

        //se suma primero el area de la nueva hoja a insertar
        cost += leafBox.Area();

        //Luego se suma la diferencia del area del virtualBox del padre tentativo con el area actual del hermano actual
        /*Si la ubicación a insertar de la nueva hoja estuviera dentro del último hermano consultado,
          la diferencia entre el area del hermano y el area del padre tentantivo (entre newLeaf y sibling),
          sería 0.
          Debido a que el area de sibling ya contiene a newLeaf, por tanto SA(virtual) es igual que SA(sibling)*/
        cost += virtualBox.Area() - sibling->box.Area();


        while (parentIndex != NULL) {
            if (parentIndex->leftChild == refitChildIndex)
                otherChild = parentIndex->rightChild;
            else otherChild = parentIndex->leftChild;

            virtualBox = otherChild->merge(virtualBox);
            cost += (virtualBox.Area() - parentIndex->box.Area());
            refitChildIndex = parentIndex;
            parentIndex = parentIndex->parentIndex;

        }

        return cost;
    }

    Node* PickBestSibling(AABB leafBox) {
        std::priority_queue<Node*, std::vector<Node*>, compare> pQueue;
        pQueue.push(rootIndex);
        Node* bestSibling = rootIndex;
        float bestInsCost = insertionCost(rootIndex, leafBox);
        float exploCost;

        while(!pQueue.empty()){
            Node* sibling = pQueue.top(); pQueue.pop();
            if (sibling->box.overlap(leafBox) && sibling->isLeaf) return NULL;

            float siblingCost = insertionCost(sibling, leafBox);
            if(siblingCost < bestInsCost){
                bestInsCost = siblingCost;
                bestSibling = sibling;
            }
            if(!sibling->isLeaf){
                exploCost = explorationCost(sibling, leafBox);
                if(exploCost < bestInsCost ){
                    pQueue.push(sibling->leftChild);
                    pQueue.push(sibling->rightChild);
                }
            }
        }
        return bestSibling;
    }
    
    void reffitingAABBs(Node* node) {
        Node* parent = node->parentIndex;
        while (parent != NULL) {
            Node* childR = parent->rightChild;
            Node* childF = parent->leftChild;
            parent->box = parent->merge(childF->box, childR->box);
            parent = parent->parentIndex;
        }
    }

    bool InsertLeaf(AABB box) { //objectIndex

        Node* bestSibling = NULL;
        Node* leafIndex = new Node(box);

        if (this->leafCount == 0) {
            this->rootIndex = leafIndex;
            leafCount++;
            leafs.push_back(leafIndex);
            return true;
        }

        bestSibling = PickBestSibling(leafIndex->box);
        if (bestSibling) {
            for (int i = 0; i < leafs.size(); i++) {
                if (leafs[i]->box.overlap(leafIndex->box)) {
                    //tImg->drawSquare(leafIndex->box.minX, leafIndex->box.minY, leafIndex->box.maxX, leafIndex->box.maxY, "red");
                    delete leafIndex;
                    return false;
               }
            }
        }
        else return false;


        createNewParent(bestSibling, leafIndex);
        reffitingAABBs(leafIndex->parentIndex);
        leafCount++;
        leafs.push_back(leafIndex);

        return true;
    }

    bool bulletCollision(AABB bullet, int pendiente, int direccion){
        
        AABB dynamicBullet;
        dynamicBullet.maxX = dynamicBullet.minX = bullet.maxX;
        dynamicBullet.maxY = dynamicBullet.minY = bullet.maxY;
        for (int i = bullet.maxX;
            (dynamicBullet.maxX < Espacio && dynamicBullet.maxX > -1) && (dynamicBullet.maxY < Espacio && dynamicBullet.maxY > -1);
            ((direccion == derecha) ? i++ : i--)) {
            tImg->drawSquare(dynamicBullet.maxX, dynamicBullet.maxY, dynamicBullet.maxX, dynamicBullet.maxY, "green");
            if (!PickBestSibling(dynamicBullet)) return true;
            dynamicBullet.maxX = dynamicBullet.minX = i;
            dynamicBullet.maxY = dynamicBullet.minY = pendiente * i - pendiente * bullet.maxX + bullet.minY;
        }
            
        
        return false;
    }

};

//PERMITE CALCULAR EN BASE A SAH, LA COMPLEJIDAD DE UN AABB
//float ComputeCost(Tree* tree)
//{
//    float cost = 0.0f;
//    float boxArea = 0.0f;
//    for (int i = 0; i < (tree->leafCount)*2 - 1; ++i)
//    {
//        if (tree->nodes[i]->isLeaf == false)
//        {
//            boxArea = tree->nodes[i]->box.Area();
//            cost += boxArea;
//        }
//    }
//    return cost;
//};

int main()
{
    srand(time(0));
    // rand();

    tImg->newImage("COLLISION.bmp");

    // rand() % 100 +1; //Generates number between 1 - 100
    Tree tree = *new Tree();
    auto start = high_resolution_clock::now();
    for (int i = 0; i < 30000; i++) {
        // AABB box = *new AABB(i * 100, 0, (i+1) * 100, (i+1) * 100);
        AABB box = *new AABB(rand() % 999 +1, rand() % 999 +1, rand() % 999 +1, rand() % 999 +1);
        //std::cout << box.minX << "  " << box.minY << "  " << box.maxX << "  " << box.maxY << std::endl;
       
        if (tree.InsertLeaf(box)) {
            //std::cout << "insertado con exito" << std::endl;
            tImg->drawSquare(box.minX, box.minY, box.maxX, box.maxY);
        }
        //else  std::cout << "no fue posible insertar la caja" << std::endl; 

    }
    tImg->drawSquare(tree.rootIndex->box.minX, tree.rootIndex->box.minY, tree.rootIndex->box.maxX , tree.rootIndex->box.maxY, "blue");
    

   /* AABB bullet;
    bullet.maxX = bullet.minX = rand()/50;
    bullet.maxY = bullet.minY = rand()/50;
    if (tree.bulletCollision(bullet,-2,1)) std::cout << "IMPACTO!" << std::endl;
    else std::cout << "ERRADO!" << std::endl;*/
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << duration.count() << " microseconds" << endl;

    tImg->save();
 
    return 0;
}

