import {ResourceSelect}            from "@derivean/resource";
import {ProducerIcon}              from "@derivean/ui";
import {
    Form,
    NumberInput
}                                  from "@use-pico/form";
import {type WithEntity}           from "@use-pico/types";
import {type FC}                   from "react";
import {ProducerSelect}            from "../input/ProducerSelect";
import {withProducerInputMutation} from "../mutation/withProducerInputMutation";
import {ProducerInputSchema}       from "../schema/input/ProducerInputSchema";
import {ProducerInputShapeSchema}  from "../schema/input/ProducerInputShapeSchema";

export namespace ProducerInputUpsertForm {
    export type Props =
        Form.PropsEx<
            withProducerInputMutation,
            ProducerInputShapeSchema,
            withProducerInputMutation["schema"]["request"],
            withProducerInputMutation["schema"]["response"]
        >
        & WithEntity.Schema.$<ProducerInputSchema>
        & {
            producerId?: string
        }
}

export const ProducerInputUpsertForm: FC<ProducerInputUpsertForm.Props> = (
    {
        entity,
        producerId,
        ...props
    }
) => {
    return <Form
        withTranslation={{
            namespace: "producer.input",
            label:     "upsert",
        }}
        icon={<ProducerIcon/>}
        withMutation={withProducerInputMutation}
        schema={ProducerInputShapeSchema}
        hidden={producerId ? ["producerId"] : undefined}
        inputs={{
            producerId: ProducerSelect,
            resourceId: ResourceSelect,
            amount:     NumberInput,
        }}
        values={{
            ...entity,
            producerId,
        }}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            producerId: "",
            resourceId: "",
            amount:     0,
        }}
        Render={({Input}) => <>
            <Input name={"producerId"}/>
            <Input name={"resourceId"}/>
            <Input name={"amount"}/>
        </>}
        {...props}
    />;
};
