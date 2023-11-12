"use client";

import {
    createSchema,
    default as CoolDiagram,
    useSchema
}                from "beautiful-react-diagrams";
import {type FC} from "react";

export namespace Diagram {
    export interface Props {
    }
}

export const Diagram: FC<Diagram.Props> = () => {
    const initialSchema = createSchema({
        nodes: [
            {
                id:          "node-1",
                content:     "Node 1",
                coordinates: [250, 60],
            },
            {
                id:          "node-2",
                content:     "Node 2",
                coordinates: [100, 200],
            },
            {
                id:          "node-3",
                content:     "Node 3",
                coordinates: [250, 220],
            },
            {
                id:          "node-4",
                content:     "Node 4",
                coordinates: [400, 200],
            },
        ],
        links: [
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
        ]
    });
    const [schema, {onChange}] = useSchema(initialSchema);

    return (
        <div style={{height: "22.5rem"}}>
            <CoolDiagram schema={schema} onChange={onChange}/>
        </div>
    );
};
