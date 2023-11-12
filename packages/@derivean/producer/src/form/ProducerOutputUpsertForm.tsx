import {ResourceSelect}   from "@derivean/resource";
import {ProducerIcon}     from "@derivean/ui";
import {NumberInput}      from "@use-pico/form";
import {t}                from "@use-pico/i18n";
import {
    ComponentProps,
    type FC
}                         from "react";
import {ProducerSelect}   from "../input/ProducerSelect";
import {ProducerOutputUI} from "../ui/ProducerOutputUI";

export namespace ProducerOutputUpsertForm {
    export type Props =
        Omit<
            ComponentProps<ProducerOutputUI["MutationForm"]>,
            "inputs" | "defaultValues" | "Render"
        >
        & {
            producerId?: string
        }
}

export const ProducerOutputUpsertForm: FC<ProducerOutputUpsertForm.Props> = (
    {
        entity,
        producerId,
        ...props
    }
) => {
    return <ProducerOutputUI.MutationForm
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
                label={t()`Produced amount`}
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