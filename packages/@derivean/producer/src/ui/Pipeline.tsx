import {IconArrowDown}            from "@tabler/icons-react";
import {
    Center,
    WithIcon
}                                 from "@use-pico/client";
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

        <Center my={"xl"}>
            <WithIcon
                color={"gray"}
                size={64}
                icon={<IconArrowDown/>}
            />
        </Center>

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
