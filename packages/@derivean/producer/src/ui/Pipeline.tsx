import {IconArrowDown}            from "@tabler/icons-react";
import {StoreProvider}            from "@use-pico/store";
import {
    Center,
    WithIcon
}                                 from "@use-pico/ui";
import {type FC}                  from "react";
import {ProducerInputQueryStore}  from "../store/ProducerInputQueryStore";
import {ProducerOutputQueryStore} from "../store/ProducerOutputQueryStore";
import {ProducerInputTable}       from "../table/ProducerInputTable";
import {ProducerOutputTable}      from "../table/ProducerOutputTable";

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
        <StoreProvider
            store={ProducerInputQueryStore}
            values={{
                where: {
                    producerId,
                },
            }}
        >
            <ProducerInputTable
                hidden={["producerId"]}
                producerId={producerId}
            />
        </StoreProvider>

        <Center my={"xl"}>
            <WithIcon
                color={"gray.5"}
                size={64}
                icon={<IconArrowDown size={64}/>}
            />
        </Center>

        <StoreProvider
            store={ProducerOutputQueryStore}
            values={{
                where: {
                    producerId,
                },
            }}
        >
            <ProducerOutputTable
                hidden={["producerId"]}
                producerId={producerId}
            />
        </StoreProvider>
    </>;
};
