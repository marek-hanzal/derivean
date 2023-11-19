"use client";

import {ProducerIcon}              from "@derivean/ui";
import {
    IconAlertTriangle,
    IconArrowRight
}                                  from "@tabler/icons-react";
import {
    Alert,
    ButtonLink,
    Group,
    Icon,
    QueryResult,
    Skeleton,
    useQueryEx
}                                  from "@use-pico/client";
import {isSchema}                  from "@use-pico/schema";
import {t}                         from "@use-pico/translator";
import {type FC}                   from "react";
import {ProducerInline}            from "../inline/ProducerInline";
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
        WithLoading={() => <Skeleton lines={4}/>}
        WithSuccess={({entity}) => {
            if (isSchema(entity, DependencyProducersSchema)) {
                return <Alert
                    color={"green"}
                    title={t()`Producer has no cyclic dependencies`}
                >
                    {entity.producers.length > 0 && <Group>
                        {entity.producers.map((producer) => <Group
                                key={producer.id}
                            >
                                <ButtonLink
                                    icon={<ProducerIcon/>}
                                    href={{
                                        href:  "/manager/producer/[id]/pipeline",
                                        query: {
                                            id: producer.id,
                                        },
                                    }}
                                    label={<ProducerInline entity={producer}/>}
                                />
                                <ProducerInput producerId={producer.id}/>
                            <Icon
                                    size={"xs"}
                                    Icon={IconArrowRight}
                                    color={"gray"}
                                />
                                <ProducerOutput producerId={producer.id}/>
                            </Group>
                        )}
                    </Group>}
                </Alert>;
            } else if (isSchema(entity, DependencyCycleSchema)) {
                return <Alert
                    icon={<IconAlertTriangle/>}
                    color={"red"}
                    title={t()`Producer has cyclic dependencies`}
                >
                    {entity.cycle.length > 0 && <Group>
                        {entity.cycle.map((producer) => <Group
                                key={producer.id}
                                gap={"gap-2"}
                            >
                                <ButtonLink
                                    icon={<ProducerIcon/>}
                                    href={{
                                        href:  "/manager/producer/[id]/pipeline",
                                        query: {
                                            id: producer.id,
                                        },
                                    }}
                                    label={producer.name}
                                />
                                <ProducerInput producerId={producer.id}/>
                            <Icon
                                    size={"xs"}
                                    Icon={IconArrowRight}
                                    color={"gray"}
                                />
                                <ProducerOutput producerId={producer.id}/>
                            </Group>
                        )}
                    </Group>}
                </Alert>;
            }
            return null;
        }}
    />;
};
