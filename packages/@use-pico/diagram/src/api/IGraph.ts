import {type ILink} from "./ILink";
import {type INode} from "./INode";

export interface IGraph {
    nodes: INode<any>[];
    links: ILink[];
}

export namespace IGraph {
    export interface Layout {
        nodes: INode.Layout<any>[];
        links: ILink[];
    }
}
