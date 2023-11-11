import {ResourceUI}      from "@derivean/resource";
import {
    NativeBreadcrumbs,
    Text
}                        from "@use-pico/ui";
import {type FC}         from "react";
import {ProducerInputUI} from "./ProducerInputUI";

export namespace ProducerInput {
    export interface Props {
        producerId: string;
    }
}

export const ProducerInput: FC<ProducerInput.Props> = (
    {
        producerId,
    }
) => {
    return <ProducerInputUI.Collection
        query={{
            where: {
                producerId,
            }
        }}
        WithSuccess={({entities}) => <>
            <NativeBreadcrumbs
                separator={"&"}
                separatorMargin={4}
            >
                {entities.map(producerInput => <ResourceUI.Fetch
                    override={producerInput.resourceId}
                    WithSuccess={({entity}) => <Text>
                        {entity.name}
                    </Text>}
                />)}
            </NativeBreadcrumbs>
        </>}
    />;
};
