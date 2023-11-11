import {ResourceUI}       from "@derivean/resource";
import {
    NativeBreadcrumbs,
    Text
}                         from "@use-pico/ui";
import {type FC}          from "react";
import {ProducerOutputUI} from "./ProducerOutputUI";

export namespace ProducerOutput {
    export interface Props {
        mark?: string[];
        producerId: string;
    }
}

export const ProducerOutput: FC<ProducerOutput.Props> = (
    {
        mark = [],
        producerId,
    }
) => {
    return <ProducerOutputUI.Collection
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
                {entities.map(entity => <ResourceUI.Fetch
                    override={entity.resourceId}
                    WithSuccess={({entity}) => <Text
                        fw={mark.includes(entity.id) ? "bold" : undefined}
                        c={mark.includes(entity.id) ? undefined : "dimmed"}
                    >
                        {entity.name}
                    </Text>}
                />)}
            </NativeBreadcrumbs>
        </>}
    />;
};
