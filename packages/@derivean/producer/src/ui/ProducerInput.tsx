import {ResourceUI}      from "@derivean/resource";
import {t}               from "@use-pico/i18n";
import {
    NativeBreadcrumbs,
    Text
}                        from "@use-pico/ui";
import {type FC}         from "react";
import {ProducerInputUI} from "./ProducerInputUI";

export namespace ProducerInput {
    export interface Props {
        mark?: string[];
        producerId: string;
    }
}

export const ProducerInput: FC<ProducerInput.Props> = (
    {
        mark = [],
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
            {entities.length > 0 && <NativeBreadcrumbs
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
            </NativeBreadcrumbs>}
            {!entities.length && <Text
                c={"dimmed"}
            >
                {t()`Producer without inputs`}
            </Text>}
        </>}
    />;
};
