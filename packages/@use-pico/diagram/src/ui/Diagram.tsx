"use client";

import {
    createSchema,
    default as CoolDiagram
} from "beautiful-react-diagrams";

import {
    type FC,
    useMemo
}                    from "react";
import {type IGraph} from "../api/IGraph";
import {withLayout}  from "../layout/withLayout";

export namespace Diagram {
    export interface Props {
        graph: IGraph;
    }
}

export const Diagram: FC<Diagram.Props> = (
    {
        graph,
    }
) => {
    const schema = useMemo(() => createSchema(withLayout(graph)), [JSON.stringify(graph)]);
    console.log(schema);

    return (
        <div style={{height: "22.5rem"}}>
            <CoolDiagram
                schema={schema}
            />
        </div>
    );
};
