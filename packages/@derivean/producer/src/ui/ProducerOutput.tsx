"use client";

import {
    ResourceInline,
    ResourceUI
}                         from "@derivean/resource";
import {t}                from "@use-pico/i18n";
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
            {entities.length > 0 && <NativeBreadcrumbs
                separator={"&"}
                separatorMargin={4}
            >
                {entities.map(entity => <ResourceUI.Fetch
                    key={entity.id}
                    override={entity.resourceId}
                    WithSuccess={({entity}) => <Text
                        fw={mark.includes(entity.id) ? "bold" : undefined}
                        c={mark.includes(entity.id) ? undefined : "dimmed"}
                    >
                        <ResourceInline entity={entity}/>
                    </Text>}
                />)}
            </NativeBreadcrumbs>}
            {!entities.length && <Text
                c={"dimmed"}
            >
                {t()`Producer without outputs`}
            </Text>}
        </>}
    />;
};
