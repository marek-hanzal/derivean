"use client";

import Dagre     from "@dagrejs/dagre";
import {
    createSchema,
    default as CoolDiagram
}                from "beautiful-react-diagrams";
import {
    Link,
    Node
}                from "beautiful-react-diagrams/@types/DiagramSchema";
import {type FC} from "react";

const g = new Dagre.graphlib.Graph().setDefaultEdgeLabel(() => ({}));

const withLayout = <TNode extends Node<any>>(nodes: TNode[], links: Link[]) => {
    g.setGraph({
        rankdir: "LR",
        align:   "DL",
        edgesep: 80,
        ranksep: 160,
        marginx: 80,
        marginy: 60,
    });
    links.forEach(edge => g.setEdge(edge.input, edge.output));
    nodes.forEach(node => g.setNode(node.id, node));
    Dagre.layout(g);
    return {
        nodes: nodes.map((node) => {
            const {
                x,
                y
            } = g.node(node.id);

            return {
                ...node,
                coordinates: [x, y]
            };
        }),
        links: links,
    };
};

export namespace Diagram {
    export interface Props {
    }
}

export const Diagram: FC<Diagram.Props> = () => {
    const schema = createSchema(
        withLayout(
            [
                {
                    id:          "node-1",
                    content:     "Node 1",
                    coordinates: [0, 0],
                },
                {
                    id:          "node-2",
                    content:     "Node 2",
                    coordinates: [0, 0],
                },
                {
                    id:          "node-3",
                    content:     "Node 3",
                    coordinates: [0, 0],
                },
                {
                    id:          "node-4",
                    content:     "Node 4",
                    coordinates: [0, 0],
                },
            ],
            [
                {
                    input:  "node-1",
                    output: "node-2"
                },
                {
                    input:  "node-1",
                    output: "node-3"
                },
                {
                    input:  "node-1",
                    output: "node-4"
                },
                {
                    input:  "node-3",
                    output: "node-4"
                },
            ]
        )
    );
    return (
        <div style={{height: "22.5rem"}}>
            <CoolDiagram
                schema={schema}
            />
        </div>
    );
};
