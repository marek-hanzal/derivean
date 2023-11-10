import {ResourceSelect}            from "@derivean/resource";
import {ProducerIcon}              from "@derivean/ui";
import {
    Form,
    NumberInput
}                                  from "@use-pico/form";
import {t}                         from "@use-pico/i18n";
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
        text={{
            submit:  entity ? t()`Update producer input` : t()`Create producer input`,
            success: entity ? {
                title:   t()`Success`,
                message: t()`Producer input has been updated successfully.`,
            } : {
                title:   t()`Success`,
                message: t()`Producer input has been created successfully.`,
            }
        }}
        icon={<ProducerIcon/>}
        withMutation={withProducerInputMutation}
        schema={ProducerInputShapeSchema}
        hidden={producerId ? ["producerId"] : undefined}
        inputs={{
            producerId: props => <ProducerSelect
                text={{
                    label:       t()`Producer name`,
                    placeholder: t()`Producer (placeholder)`,
                }}
                {...props}
            />,
            resourceId: props => <ResourceSelect
                text={{
                    label:       t()`Resource name`,
                    placeholder: t()`Resource (placeholder)`,
                }}
                {...props}
            />,
            amount:     props => <NumberInput
                label={t()`Consumed amount`}
                {...props}
            />,
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
