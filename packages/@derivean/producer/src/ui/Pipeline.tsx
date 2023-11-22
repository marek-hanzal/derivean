import {type FC}                  from "react";
import {ProducerInputQueryStore}  from "../store/ProducerInputQueryStore";
import {ProducerOutputQueryStore} from "../store/ProducerOutputQueryStore";

export namespace Pipeline {
    export interface Props {
        producerId?: string;
    }
}

export const Pipeline: FC<Pipeline.Props> = (
    {
        producerId,
    }
) => {
    return <>
        <ProducerInputQueryStore.Provider
            values={{
                where: {
                    producerId,
                },
            }}
        >
            {/*<ProducerInputTable*/}
            {/*    hidden={["producerId"]}*/}
            {/*    producerId={producerId}*/}
            {/*/>*/}
        </ProducerInputQueryStore.Provider>

        <div className={"my-24"}>
            <div className={"i-tabler-arrow-narrow-down text-zinc-400 h-xl w-xl"}/>
        </div>

        <ProducerOutputQueryStore.Provider
            values={{
                where: {
                    producerId,
                },
            }}
        >
            {/*<ProducerOutputTable*/}
            {/*    hidden={["producerId"]}*/}
            {/*    producerId={producerId}*/}
            {/*/>*/}
        </ProducerOutputQueryStore.Provider>
    </>;
};
