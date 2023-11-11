import {ResourceUI}        from "@derivean/resource";
import {NativeBreadcrumbs} from "@use-pico/ui";
import {type FC}           from "react";
import {ProducerOutputUI}  from "./ProducerOutputUI";

export namespace ProducerOutput {
    export interface Props {
        referenceId?: string;
        producerId: string;
    }
}

export const ProducerOutput: FC<ProducerOutput.Props> = (
    {
        referenceId,
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
                    WithSuccess={({entity}) => entity.name}
                />)}
            </NativeBreadcrumbs>
        </>}
    />;
};
