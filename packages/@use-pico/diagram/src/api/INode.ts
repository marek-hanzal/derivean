import {type Node} from "beautiful-react-diagrams/@types/DiagramSchema";

export type INode<TData> = Omit<Node<TData>, "coordinates">;

export namespace INode {
    export type Layout<TData> = Node<TData>;
}
