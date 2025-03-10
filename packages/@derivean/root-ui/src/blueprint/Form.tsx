/** @format */

import { BlueprintIcon } from "@derivean/ui";
import { BlueprintSchema } from "@derivean/utils";
import { zodResolver } from "@hookform/resolvers/zod";
import {
	Button,
	FormCss,
	FormError,
	FormInput,
	onSubmit,
	Tx,
	type Form as CoolForm,
} from "@use-pico/client";
import { type FC } from "react";
import { useForm } from "react-hook-form";

namespace Form {
	export interface Props extends CoolForm.Props<BlueprintSchema["shape"]> {
		//
	}
}

const Form: FC<Form.Props> = ({ mutation, defaultValues, variant, tva = FormCss, css }) => {
	const form = useForm<BlueprintSchema["~shape"]>({
		resolver: zodResolver(BlueprintSchema.shape),
		defaultValues: { cycles: 1, sort: 0, limit: 1, ...defaultValues },
	});

	const tv = tva({
		...variant,
		isLoading: form.formState.isLoading,
		isSubmitting: form.formState.isSubmitting,
		css,
	}).slots;

	return (
		<form
			className={tv.base()}
			onSubmit={onSubmit({ form, mutation })}
		>
			<FormError
				variant={{ highlight: true }}
				error={form.formState.errors.root}
			/>

			<FormInput
				formState={form.formState}
				name={"name"}
				label={<Tx label={"Building name (label)"} />}
				required
			>
				<input
					type={"text"}
					className={tv.input()}
					{...form.register("name")}
				/>
			</FormInput>

			<FormInput
				formState={form.formState}
				name={"cycles"}
				label={<Tx label={"Construction cycles (label)"} />}
				hint={<Tx label={"Amount of cycles to build this building (hint)"} />}
				required
			>
				<input
					type={"number"}
					className={tv.input()}
					min={1}
					{...form.register("cycles")}
				/>
			</FormInput>

			<FormInput
				formState={form.formState}
				name={"limit"}
				label={<Tx label={"Building limit (label)"} />}
				hint={<Tx label={"Maximum number of building a player can build (hint)"} />}
				required
			>
				<input
					type={"number"}
					className={tv.input()}
					min={1}
					{...form.register("limit")}
				/>
			</FormInput>

			<FormInput
				formState={form.formState}
				name={"sort"}
				label={<Tx label={"Blueprint sort (label)"} />}
				hint={<Tx label={"Sort blueprints by this number (hint)"} />}
				required
			>
				<input
					type={"number"}
					className={tv.input()}
					min={0}
					{...form.register("sort")}
				/>
			</FormInput>

			<FormInput
				formState={form.formState}
				name={"image"}
				label={<Tx label={"Building image (label)"} />}
				hint={<Tx label={"Building image (hint)"} />}
			>
				<input
					type={"file"}
					className={tv.input()}
					{...form.register("image")}
				/>
			</FormInput>

			<div className={"flex flex-row justify-between gap-8"}>
				<Button
					iconEnabled={BlueprintIcon}
					type={"submit"}
				>
					<Tx label={"Save (submit)"} />
				</Button>
			</div>
		</form>
	);
};

export { Form as BlueprintForm };
