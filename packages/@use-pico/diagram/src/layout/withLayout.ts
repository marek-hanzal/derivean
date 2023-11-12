import Dagre         from "@dagrejs/dagre";
import {type IGraph} from "../api/IGraph";

export const withLayout = (
    {
        nodes,
        links
    }: IGraph
): IGraph.Layout => {
    const g = new Dagre.graphlib.Graph().setDefaultEdgeLabel(() => ({}));
    g.setGraph({
        rankdir: "RL",
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
