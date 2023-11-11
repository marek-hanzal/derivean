"use client";

import {ProducerIcon}              from "@derivean/ui";
import {
    IconAlertTriangle,
    IconArrowRight
}                                  from "@tabler/icons-react";
import {t}                         from "@use-pico/i18n";
import {
    QueryResult,
    useQueryEx
}                                  from "@use-pico/query";
import {isSchema}                  from "@use-pico/schema";
import {
    Alert,
    ButtonLink,
    Group,
    Loader,
    NativeBreadcrumbs,
    WithIcon
}                                  from "@use-pico/ui";
import {type FC}                   from "react";
import {withDependenciesQuery}     from "../query/withDependenciesQuery";
import {DependencyCycleSchema}     from "../schema/DependencyCycleSchema";
import {DependencyProducersSchema} from "../schema/DependencyProducersSchema";
import {ProducerInput}             from "./ProducerInput";
import {ProducerOutput}            from "./ProducerOutput";

export namespace Dependencies {
    export interface Props {
        producerId: string;
    }
}

export const Dependencies: FC<Dependencies.Props> = (
    {
        producerId,
    }
) => {
    const result = useQueryEx({
        withQuery: withDependenciesQuery,
        request:   {
            id: producerId,
        },
    });

    return <QueryResult
        result={result}
        WithLoading={() => <Loader size={"sm"} type={"dots"}/>}
        WithSuccess={({entity}) => {
            if (isSchema(entity, DependencyProducersSchema)) {
                return <Alert
                    color={"green"}
                    title={t()`Producer has no cyclic dependencies`}
                >
                    <NativeBreadcrumbs>
                        {entity.producers.map((producer) => <ButtonLink
                                key={producer.id}
                                icon={<ProducerIcon/>}
                                href={{
                                    href:  "/manager/producer/[id]/pipeline",
                                    query: {
                                        id: producer.id,
                                    },
                                }}
                                label={producer.name}
                            />
                        )}
                    </NativeBreadcrumbs>
                </Alert>;
            } else if (isSchema(entity, DependencyCycleSchema)) {
                return <Alert
                    icon={<IconAlertTriangle/>}
                    color={"red.5"}
                    title={t()`Producer has cyclic dependencies`}
                >
                    <NativeBreadcrumbs>
                        {entity.cycle.map((producer) => <Group gap={2}>
                                <ButtonLink
                                    key={producer.id}
                                    icon={<ProducerIcon/>}
                                    href={{
                                        href:  "/manager/producer/[id]/pipeline",
                                        query: {
                                            id: producer.id,
                                        },
                                    }}
                                    label={producer.name}
                                />
                                (
                                <ProducerInput producerId={producer.id}/>
                                <WithIcon
                                    size={"xs"}
                                    icon={<IconArrowRight/>}
                                    color={"gray.8"}
                                />
                                <ProducerOutput producerId={producer.id}/>
                                )
                            </Group>
                        )}
                    </NativeBreadcrumbs>
                </Alert>;
            }
            return null;
        }}
    />;
};
